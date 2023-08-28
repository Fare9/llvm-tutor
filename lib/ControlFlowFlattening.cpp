#include "ControlFlowFlattening.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Local.h>

#include <unordered_map>

using namespace llvm;

namespace {

std::unordered_map<const BasicBlock*, std::string> bb_names;

/// https://stackoverflow.com/questions/26281823/llvm-how-to-get-the-label-of-basic-blocks
static std::string getSimpleNodeLabel(const BasicBlock *Node) {
  if (!Node->getName().empty())
    return Node->getName().str();

  if (bb_names.find(Node) != bb_names.end())
    return bb_names[Node];

  std::string Str;
  raw_string_ostream OS(Str);

  Node->printAsOperand(OS, false);

  bb_names[Node] = OS.str();

  return bb_names[Node];
}

static void demotePhiNodes(Function &F) {
  SmallVector<PHINode *, 4> phiNodes;
  do {
    phiNodes.clear();

    for (auto &BB : F) {
      for (auto &I : BB.phis())
        phiNodes.emplace_back(&I);
    }

    for (auto *phi : phiNodes)
      /// function takes a virtual register computed by a phi node
      /// and replaces it with a slot in the stack frame
      DemotePHIToStack(phi, F.begin()->getTerminator());
  } while (!phiNodes.empty());
}

/// Print the function's basic blocks and instructions.
void printFunction(Function &F) {
  outs() << "Function: " << F.getName() << "\n";
  for (auto &BB : F) {
    outs() << "BB-" << getSimpleNodeLabel(&BB) << "\n";
    for (auto &I : BB)
      outs() << "\t" << I << "\n";
  }
}

SmallVector<BasicBlock *, 20> getBlocksToFlatten(Function &F) {
  SmallVector<BasicBlock *, 20> flattenedBB;

  auto *entryBlock = &(F.getEntryBlock());

  for (auto &BB : F) {
    if (&BB == entryBlock) {
      outs() << "Not flattening entry block " << getSimpleNodeLabel(&BB)
             << "\n";
      continue;
    }

    outs() << "Adding block to flatten: " << getSimpleNodeLabel(&BB) << '\n';
    flattenedBB.emplace_back(&BB);
  }

  return flattenedBB;
}

/// Create an alloca instruction for a global variable and then
/// store an initial value into the allocated variable.
AllocaInst *initDispatchVar(Function &F, int initialValue) {
  BasicBlock &EntryBlock = F.getEntryBlock();
  IRBuilder<> EntryBuilder(&EntryBlock, EntryBlock.begin());
  AllocaInst *DispatchVar = EntryBuilder.CreateAlloca(EntryBuilder.getInt32Ty(),
                                                      nullptr, "dispatch_var");
  EntryBuilder.CreateStore(
      ConstantInt::get(EntryBuilder.getInt32Ty(), initialValue), DispatchVar);

  return DispatchVar;
}

/// Create a new block with the instructions of the entry block
/// but with an unconditional jump to a newer block.
BasicBlock &splitBranchOffEntryBlock(Function &F) {
  BasicBlock &entryBlockTail = F.getEntryBlock();
  BasicBlock *pNewEntryBlock =
      entryBlockTail.splitBasicBlockBefore(entryBlockTail.getTerminator(), "");
  // avoid warning
  (void)pNewEntryBlock;

  return entryBlockTail;
}

BasicBlock *insertDispatchBlockAfterEntryBlock(Function &F,
                                               AllocaInst *DispatchVar) {
  BasicBlock &EntryBlock = F.getEntryBlock();
  auto *br = dyn_cast<BranchInst>(EntryBlock.getTerminator());
  auto Successor = br->getSuccessor(0);

  // First create a DispatchBlock where we will insert
  // the checks of the dispatch variable

  // then connect in the next way:
  // EntryBlock -> DispatchBlock
  // DispatchBlock -> Successor
  BasicBlock *DispatchBlock =
      BasicBlock::Create(F.getContext(), "dispatch_block", &F);
  IRBuilder<> DispatchBuilder(DispatchBlock, DispatchBlock->begin());
  DispatchBuilder.CreateBr(Successor);

  br->setSuccessor(0, DispatchBlock);
  DispatchBlock->moveAfter(&EntryBlock);

  return DispatchBlock;
}

void flattenBlock(BasicBlock *block, int &dispatchVal, Function &F,
                  AllocaInst *DispatchVar, BasicBlock *DispatchBlock) {
  /// only handle branches for now
  outs() << "Flattening block " << getSimpleNodeLabel(block) << '\n';
  if (auto br = dyn_cast<BranchInst>(block->getTerminator())) {
    for (unsigned i = 0, e = br->getNumSuccessors(); i < e; ++i) {
      // get the successor of the branch
      BasicBlock *Successor = br->getSuccessor(i);

      // Create a detour block for the next operation:
      // DispatchVar = X
      // jmp DispatchBlock
      BasicBlock *DetourBlock = BasicBlock::Create(F.getContext(), "", &F);
      IRBuilder<> Builder(DetourBlock);
      Builder.CreateStore(ConstantInt::get(Builder.getInt32Ty(), ++dispatchVal),
                          DispatchVar);
      Builder.CreateBr(DispatchBlock);

      // insert the detour block after the current one
      br->setSuccessor(i, DetourBlock);
      DetourBlock->moveAfter(block);

      // Add a new branch in the dispatcher, this branch will be inserted
      // adding a new block, and a new comparison so in case the dispatch
      // variable is equals to the dispatch value, jump to the correct case
      Instruction *FirstInst = DispatchBlock->getFirstNonPHI();
      IRBuilder<> DispatchBuilder(FirstInst);
      LoadInst *loadSwitchVar = DispatchBuilder.CreateLoad(
          DispatchBuilder.getInt32Ty(), DispatchVar, "dispatch_var");
      auto *Cond = DispatchBuilder.CreateICmpEQ(
          ConstantInt::get(DispatchBuilder.getInt32Ty(), dispatchVal),
          loadSwitchVar);
      SplitBlockAndInsertIfThen(Cond, FirstInst, false, nullptr,
                                (DomTreeUpdater *)nullptr, nullptr, Successor);
    }
  }
}

bool flattenFunction(Function &F) {
  outs() << "Flattening " << F.getName() << "\n";
  outs() << F.size() << " blocks\n";
  outs() << F.getInstructionCount() << " instructions\n";

  if (F.empty() || F.size() == 1) {
    outs() << "No Basic Block or just one Basic Block, Skipping\n";
    return false;
  }

  if (F.getInstructionCount() < 1) {
    outs() << "Only one instruction, Skipping\n";
    return false;
  }

  int dispatchVal = 0x1001;

  printFunction(F);
  auto flattenedBB = getBlocksToFlatten(F);
  demotePhiNodes(F);
  AllocaInst *DispatchVar = initDispatchVar(F, dispatchVal);
  BasicBlock &entryBlockTail = splitBranchOffEntryBlock(F);
  flattenedBB.push_back(&entryBlockTail);
  BasicBlock *DispatchBlock =
      insertDispatchBlockAfterEntryBlock(F, DispatchVar);

  for (auto pToflatBB : flattenedBB)
    flattenBlock(pToflatBB, dispatchVal, F, DispatchVar, DispatchBlock);

  outs() << "Function flattened:\n";
  printFunction(F);

  return true;
}

} // namespace

/// Function to run on given module
llvm::PreservedAnalyses
ControlFlowFlattening::run(llvm::Module &Module,
                           llvm::ModuleAnalysisManager &MAM) {
  for (Function &F : Module)
    ::flattenFunction(F);
  return PreservedAnalyses::none();
}

bool LegacyControlFlowFlattening::runOnModule(llvm::Module &M) {
  for (Function &F : M)
    ::flattenFunction(F);
  return true;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
PassPluginLibraryInfo getPassPluginInfo() {
  static std::atomic<bool> ONCE_FLAG(false);
  return {LLVM_PLUGIN_API_VERSION, "control flow flattening", "0.0.1",
          [](PassBuilder &PB) {
            try {
              // #1 REGISTRATION FOR "opt -passes=control-flow-flattening"
              PB.registerPipelineParsingCallback(
                  [&](StringRef Name, ModulePassManager &MPM,
                      ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "control-flow-flattening") {
                      MPM.addPass(ControlFlowFlattening());
                      return true;
                    }
                    return false;
                  });
              // #2 REGISTRATION FOR optimization pipeline
              PB.registerPipelineEarlySimplificationEPCallback(
                  [&](ModulePassManager &MPM, OptimizationLevel opt) {
                    if (ONCE_FLAG) {
                      return true;
                    }
                    MPM.addPass(ControlFlowFlattening());
                    ONCE_FLAG = true;
                    return true;
                  });
            } catch (const std::exception &e) {
              errs() << "Error registering the pass: " << e.what() << '\n';
            }
          }};
};

/// In order to be obtain information of the pass from other tools
extern "C" __attribute__((visibility("default")))
LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getPassPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyControlFlowFlattening::ID = 0;

static RegisterPass<LegacyControlFlowFlattening>
    X(/*PassArg=*/"legacy-cff",
      /*Name=*/"LegacyControlFlowFlattening",
      /*CFGOnly=*/false,
      /*is_analysis=*/false);