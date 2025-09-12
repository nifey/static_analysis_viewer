#ifndef LLVM_TRACE_WRITER_H
#define LLVM_TRACE_WRITER_H

#include <iostream>
#include <string>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include <unordered_map>
#include <utility>
#include <cstddef>
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include <sstream>
#include <memory>
#include <functional>

namespace std
{
    template <>
    struct hash<std::pair<std::string, std::string>>
    {
        std::size_t operator()(const std::pair<std::string, std::string> &p) const noexcept
        {
            return std::hash<std::string>()(p.first) ^ (std::hash<std::string>()(p.second) << 1);
        }
    };
}

namespace llvm
{
    namespace analysis_trace
    {

        class TraceWriter
        {
        private:
            raw_ostream &OS;
            std::string CurrentGroup;
            std::unordered_map<const Value *, std::string> NodeIds;
            std::unordered_map<std::string, std::string> LastNodeInfo;
            std::unordered_map<std::pair<std::string, std::string>, std::string,
                               std::hash<std::pair<std::string, std::string>>>
                LastEdgeInfo;
            std::string LastGlobalInfo;
            unsigned NodeCounter;

            std::string getNodeId(const Value *val)
            {
                if (!val)
                    return "null";

                auto It = NodeIds.find(val);
                if (It != NodeIds.end())
                {
                    return It->second;
                }

                std::string Id;
                if (const BasicBlock *bb = dyn_cast<BasicBlock>(val))
                {
                    Id = "BB_" + bb->getName().str();
                    if (Id == "BB_")
                    {
                        Id = "BB_" + std::to_string(NodeCounter++);
                    }
                }
                else if (isa<Instruction>(val))
                {
                    Id = "I_" + std::to_string(NodeCounter++);
                }

                else if (const Function *F = dyn_cast<Function>(val))
                {
                    Id = "F_" + F->getName().str();
                }
                else
                {
                    Id = "V_" + std::to_string(NodeCounter++);
                }

                NodeIds[val] = Id;
                return Id;
            }

            std::string formatNodeId(const std::string &nodeId, bool includeGroup = true)
            {
                if (includeGroup && !CurrentGroup.empty())
                {
                    return CurrentGroup + ":" + nodeId;
                }
                return nodeId;
            }

        public:
            explicit TraceWriter(raw_ostream &OutStream)
                : OS(OutStream), CurrentGroup(), NodeIds(), LastNodeInfo(), LastEdgeInfo(), LastGlobalInfo(), NodeCounter(0) {}

            void selectGroup(const std::string &groupName)
            {
                CurrentGroup = groupName;
                OS << ">>globalinfo " << groupName << "\n";
                std::string seedNodeId = "GroupAnchor_" + groupName;
                OS << ">>node " << formatNodeId(seedNodeId) << "\n";

                OS << ">>nodeinfo " << formatNodeId(seedNodeId) << " entry\n";
            }

            void defineNode(const Value *val, const std::string &content = "")
            {
                std::string nodeId = getNodeId(val);
                OS << ">>node " << formatNodeId(nodeId) << "\n";

                if (!content.empty())
                {
                    OS << content;
                }
                else if (const BasicBlock *bb = dyn_cast<BasicBlock>(val))
                {

                    for (const Instruction &I : *bb)
                    {
                        std::string istring;
                        raw_string_ostream InstOS(istring);
                        I.print(InstOS);
                        OS << InstOS.str() << "\n";
                    }
                }
                else if (const Instruction *I = dyn_cast<Instruction>(val))
                {

                    std::string istring;
                    raw_string_ostream InstOS(istring);
                    I->print(InstOS);
                    OS << InstOS.str();
                }
                else if (const Function *F = dyn_cast<Function>(val))
                {
                    OS << "Function: " << F->getName().str();
                }
                OS << "\n";
            }

            void defineEdge(const Value *Src, const Value *Dst)
            {
                std::string srcId = formatNodeId(getNodeId(Src));
                std::string dstId = formatNodeId(getNodeId(Dst));
                OS << ">>edge " << srcId << " " << dstId << "\n";
            }

            template <typename T, typename F>
            void recordInfoAtNode(const T &data, const Value *location,
                                  F serializer,
                                  const std::string &comment = "")
            {
                std::string nodeId = formatNodeId(getNodeId(location));
                std::string info = serializer(data);

                auto key = nodeId + comment;
                if (LastNodeInfo[key] == info)
                {

                    OS << ">>prevnodeinfo " << nodeId;
                    if (!comment.empty())
                    {
                        OS << " " << comment;
                    }
                    OS << "\n";
                }
                else
                {

                    OS << ">>nodeinfo " << nodeId;
                    if (!comment.empty())
                    {
                        OS << " " << comment;
                    }
                    OS << "\n"
                       << info << "\n";
                    LastNodeInfo[key] = info;
                }
            }

            void recordInfoAtNode(const std::string &info, const Value *location,
                                  const std::string &comment = "")
            {

                recordInfoAtNode<std::string>(info, location, [](const std::string &s)
                                              { return s; }, comment);
            }

            template <typename T, typename F>
            void recordInfoOnEdge(const T &data, const Value *srcnode, const Value *dstnode,
                                  F serializer,
                                  const std::string &comment = "")
            {
                std::string srcId = formatNodeId(getNodeId(srcnode));
                std::string dstId = formatNodeId(getNodeId(dstnode));
                std::string info = serializer(data);

                auto key = std::make_pair(srcId + dstId, comment);
                if (LastEdgeInfo[key] == info)
                {
                    OS << ">>prevedgeinfo " << srcId << " " << dstId;
                    if (!comment.empty())
                    {
                        OS << " " << comment;
                    }
                    OS << "\n";
                }
                else
                {
                    OS << ">>edgeinfo " << srcId << " " << dstId;
                    if (!comment.empty())
                    {
                        OS << " " << comment;
                    }
                    OS << "\n"
                       << info << "\n";
                    LastEdgeInfo[key] = info;
                }
            }

            void recordInfoOnEdge(const std::string &info, const Value *srcnode,
                                  const Value *dstnode, const std::string &comment = "")
            {
                recordInfoOnEdge<std::string>(info, srcnode, dstnode, [](const std::string &s)
                                              { return s; }, comment);
            }

            void recordGlobalInfo(const std::string &info, const std::string &comment = "")
            {
                if (LastGlobalInfo == info + comment)
                {
                    OS << ">>prevglobalinfo";
                    if (!comment.empty())
                    {
                        OS << " " << comment;
                    }
                    OS << "\n";
                }
                else
                {
                    OS << ">>globalinfo";
                    if (!comment.empty())
                    {
                        OS << " " << comment;
                    }
                    OS << "\n"
                       << info << "\n";
                    LastGlobalInfo = info + comment;
                }
            }

            void recordControlFlowGraph(Function *F)
            {
                if (!F || F->empty())
                    return;

                selectGroup(F->getName().str());

                for (BasicBlock &bb : *F)
                {
                    defineNode(&bb);
                }

                for (BasicBlock &bb : *F)
                {
                    for (BasicBlock *Succ : successors(&bb))
                    {
                        defineEdge(&bb, Succ);
                    }
                }
            }

            void recordUseDefChains(Function *F)
            {
                if (!F)
                    return;

                selectGroup(F->getName().str() + "_usedef");

                for (BasicBlock &bb : *F)
                {
                    for (Instruction &I : bb)
                    {
                        if (!I.getType()->isVoidTy())
                        {
                            defineNode(&I);

                            for (User *U : I.users())
                            {
                                if (Instruction *UserInst = dyn_cast<Instruction>(U))
                                {
                                    defineEdge(&I, UserInst);
                                }
                            }
                        }
                    }
                }
            }

            void recordCallGraph(Module *M)
            {
                if (!M)
                    return;

                selectGroup("CallGraph");

                for (Function &F : *M)
                {
                    if (!F.isDeclaration())
                    {
                        defineNode(&F);
                    }
                }

                for (Function &F : *M)
                {
                    if (F.isDeclaration())
                        continue;

                    for (BasicBlock &bb : F)
                    {
                        for (Instruction &I : bb)
                        {
                            if (CallInst *CI = dyn_cast<CallInst>(&I))
                            {
                                if (Function *Callee = CI->getCalledFunction())
                                {
                                    defineEdge(&F, Callee);
                                }
                            }
                            else if (InvokeInst *II = dyn_cast<InvokeInst>(&I))
                            {
                                if (Function *Callee = II->getCalledFunction())
                                {
                                    defineEdge(&F, Callee);
                                }
                            }
                        }
                    }
                }
            }

            void defineInterproceduralEdge(const Value *srcin1, const std::string &g1,
                                           const Value *dstin2, const std::string &g2)
            {
                std::string srcId = g1 + ":" + getNodeId(srcin1);
                std::string dstId = g2 + ":" + getNodeId(dstin2);
                OS << ">>edge " << srcId << " " << dstId << "\n";
            }
        };

    }
}

#endif