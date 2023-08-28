//==============================================================================
// FILE:
//      ControlFlowFlattening.h
//
// DESCRIPTION:
//      Given an IR file it will take each one of the functions and will destroy
//      its CFG to create one based on switch instruction.
//==============================================================================

#ifndef LLVM_TUTOR_CONTROLFLOWFLATTENING_H
#define LLVM_TUTOR_CONTROLFLOWFLATTENING_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

struct ControlFlowFlattening : llvm::PassInfoMixin<ControlFlowFlattening>
{
    /// This function runs the control flow flattening for all the
    /// functions inside of a module.
    llvm::PreservedAnalyses run(llvm::Module &Module,
                                llvm::ModuleAnalysisManager &MAM);
};

struct LegacyControlFlowFlattening : public llvm::ModulePass {
    static char ID;
    LegacyControlFlowFlattening() : ModulePass(ID) {}
    bool runOnModule(llvm::Module &M) override;
};

#endif // !LLVM_TUTOR_CONTROLFLOWFLATTENING_H