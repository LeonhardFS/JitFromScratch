#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>

#include "JitFromScratch.h"

using namespace llvm;

Expected<std::string> codegenIR(Module &module, unsigned items) {
  return "todo";
}

// Determine the size of a C array at compile-time.
template <typename T, size_t sizeOfArray>
constexpr unsigned arrayElements(T (&)[sizeOfArray]) {
  return sizeOfArray;
}

// This function will be called from JITed code.
int *customIntAllocator(unsigned items) {
  static int memory[100];
  static unsigned allocIdx = 0;

  if (allocIdx + items > arrayElements(memory))
    exit(-1);

  int *block = memory + allocIdx;
  allocIdx += items;

  return block;
}

// This function will be replaced by a runtime-time compiled version.
template <size_t sizeOfArray>
int *integerDistances(const int (&x)[sizeOfArray], int *y) {
  unsigned items = arrayElements(x);
  int *results = customIntAllocator(items);

  for (int i = 0; i < items; i++) {
    results[i] = abs(x[i] - y[i]);
  }

  return results;
}

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);

  ExitOnError ExitOnErr;
  ExitOnErr.setBanner("JitFromScratch: ");

  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  int x[]{0, 1, 2};
  int y[]{3, 1, -1};

  JitFromScratch Jit(ExitOnErr);

  auto C = std::make_unique<LLVMContext>();
  auto M = std::make_unique<Module>("JitFromScratch", *C);
  M->setDataLayout(Jit.getDataLayout());

  ExitOnErr(codegenIR(*M, arrayElements(x)));
  ExitOnErr(Jit.submitModule(std::move(M), std::move(C)));

  int *z = integerDistances(x, y);

  outs() << format("Integer Distances: %d, %d, %d\n\n", z[0], z[1], z[2]);
  outs().flush();

  return 0;
}
