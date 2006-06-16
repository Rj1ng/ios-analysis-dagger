//===- Interpreter.cpp - Top-Level LLVM Interpreter Implementation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the top-level functionality for the LLVM interpreter.
// This interpreter is designed to be a very simple, portable, inefficient
// interpreter.
//
//===----------------------------------------------------------------------===//

#include "Interpreter.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/ModuleProvider.h"
using namespace llvm;

static struct RegisterInterp {
  RegisterInterp() { Interpreter::Register(); }
} InterpRegistrator;

namespace llvm {
  void LinkInInterpreter() {
  }
}

/// create - Create a new interpreter object.  This can never fail.
///
ExecutionEngine *Interpreter::create(ModuleProvider *MP) {
  Module *M;
  try {
    M = MP->materializeModule();
  } catch (...) {
    return 0;  // error materializing the module.
  }
  
  if (M->getEndianness() == Module::AnyEndianness) {
    int Test = 0;
    *(char*)&Test = 1;    // Return true if the host is little endian
    bool isLittleEndian = (Test == 1);
    M->setEndianness(isLittleEndian ? Module::LittleEndian : Module::BigEndian);
  }

  if (M->getPointerSize() == Module::AnyPointerSize) {
    // Follow host.
    bool Ptr64 = sizeof(void*) == 8;
    M->setPointerSize(Ptr64 ? Module::Pointer64 : Module::Pointer32);
  }

  return new Interpreter(M);
}

//===----------------------------------------------------------------------===//
// Interpreter ctor - Initialize stuff
//
Interpreter::Interpreter(Module *M) : ExecutionEngine(M), TD("lli", M) {
      
  memset(&ExitValue, 0, sizeof(ExitValue));
  setTargetData(&TD);
  // Initialize the "backend"
  initializeExecutionEngine();
  initializeExternalFunctions();
  emitGlobals();

  IL = new DefaultIntrinsicLowering();
}

Interpreter::~Interpreter() {
  delete IL;
}

void Interpreter::runAtExitHandlers () {
  while (!AtExitHandlers.empty()) {
    callFunction(AtExitHandlers.back(), std::vector<GenericValue>());
    AtExitHandlers.pop_back();
    run();
  }
}

/// run - Start execution with the specified function and arguments.
///
GenericValue
Interpreter::runFunction(Function *F,
                         const std::vector<GenericValue> &ArgValues) {
  assert (F && "Function *F was null at entry to run()");

  // Try extra hard not to pass extra args to a function that isn't
  // expecting them.  C programmers frequently bend the rules and
  // declare main() with fewer parameters than it actually gets
  // passed, and the interpreter barfs if you pass a function more
  // parameters than it is declared to take. This does not attempt to
  // take into account gratuitous differences in declared types,
  // though.
  std::vector<GenericValue> ActualArgs;
  const unsigned ArgCount = F->getFunctionType()->getNumParams();
  for (unsigned i = 0; i < ArgCount; ++i)
    ActualArgs.push_back(ArgValues[i]);

  // Set up the function call.
  callFunction(F, ActualArgs);

  // Start executing the function.
  run();

  return ExitValue;
}

