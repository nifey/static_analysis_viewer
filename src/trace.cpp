/*
 * Trace file parsing, traversal and rendering
 */
#include "trace.h"
#include <iostream>
#include <fstream>

using namespace std;

namespace sail {

    // Splits the given string into two on the first occurrence of delimiter
    pair<string, string> splitOnFirst(string str, string delimiter) {
        auto separatorPos = str.find_first_of(delimiter);
        if (separatorPos == string::npos)
            return make_pair(str,"");
        else
            return make_pair(
                    str.substr(0, separatorPos),
                    str.substr(separatorPos+1));
    }

    // Splits the given string on delimiter
    vector<string> splitOn(string str, string delimiter) {
        vector<string> tokens;
        auto separatorPos = str.find_first_of(delimiter);
        while (separatorPos != string::npos) {
            tokens.push_back(str.substr(0, separatorPos));
            str = str.substr(separatorPos+1);
            separatorPos = str.find_first_of(delimiter);
        }
        tokens.push_back(str);
        return tokens;
    }

    void Graph::addNode(string nodeName, std::string nodeContent) {
        auto splitName = splitOnFirst(nodeName, ":");
        string groupName = splitName.first;
        string subNodeName = splitName.second;

        if (nodeIDs.find(groupName) == nodeIDs.end())
            nodeIDs[groupName];
        if (nodeIDs[groupName].find(subNodeName) != nodeIDs[groupName].end()) {
            NodeID nodeID = nodeIDs[groupName][subNodeName];
            if (nodeContents.find(nodeID) != nodeContents.end()) {
                cout << "Node " << nodeName << " is already defined with content\n" <<
                    nodeContents[nodeID] << "\nFound redefinition\n";
                exit(0);
            }
            nodeContents[nodeID] = nodeContent;
            return;
        }
        NodeID nodeID = nodeNames.size();
        nodeNames.push_back(nodeName);
        nodeIDs[groupName][subNodeName] = nodeID;
        nodeContents[nodeID] = nodeContent;
    }

    void Graph::addEdge(string srcNodeName, string dstNodeName) {
        NodeID srcNodeID = getNodeID(srcNodeName);
        NodeID dstNodeID = getNodeID(dstNodeName);
        if (edges.find(srcNodeID) == edges.end())
            edges[srcNodeID];
        if (edges[srcNodeID].find(dstNodeID) == edges[srcNodeID].end())
            edges[srcNodeID].insert(dstNodeID);
    }

    // Returns the ID for the given node.
    // If the node has not been seen before, a new ID is assigned
    NodeID Graph::getNodeID(string nodeName) {
        auto splitName = splitOnFirst(nodeName, ":");
        string groupName = splitName.first;
        string subNodeName = splitName.second;

        if (nodeIDs.find(groupName) == nodeIDs.end())
            nodeIDs[groupName];
        if (nodeIDs[groupName].find(subNodeName) == nodeIDs[groupName].end()) {
            // Node not seen before, create a new node
            NodeID nodeID = nodeNames.size();
            nodeNames.push_back(nodeName);
            nodeIDs[groupName][subNodeName] = nodeID;
            return nodeID;
        }
        return nodeIDs[groupName][subNodeName];
    }

    std::string Graph::getNodeName(NodeID nodeID) {
        return nodeNames[nodeID];
    }

    std::string Graph::getNodeContents(NodeID nodeID) {
        return nodeContents[nodeID];
    }

    void Trace::processInstruction(string currentInstruction) {
        auto splitInstruction = splitOnFirst(currentInstruction, "\n");
        string instructionHeader = splitInstruction.first;
        string instructionBody = splitInstruction.second;

        auto headerTokens = splitOn(instructionHeader, " \t");
        string instruction = headerTokens[0];
        if (instruction == ">>node") {
            if (headerTokens.size() != 2) {
                cout << "Invalid number of arguments in >>node instruction: " << instructionHeader << "\n";
                exit(0);
            }
            this->graph.addNode(headerTokens[1], instructionBody);
        } else if (instruction == ">>edge") {
            if (headerTokens.size() != 3) {
                cout << "Invalid number of arguments in >>edge instruction: " << instructionHeader << "\n";
                exit(0);
            }
            if (instructionBody != "") {
                cout << ">>edge instruction does not take any data argument\n" << instructionBody << "\n";
                exit(0);
            }
            this->graph.addEdge(headerTokens[1], headerTokens[2]);
        } else {
            cout << "Unknown instruction " << instruction << " found in " << instructionHeader << "\n";
            exit(0);
        }
    }

    Trace::Trace(string _filename) {
        this->filename = _filename;
        cout << "Reading tracefile : " << this->filename << "\n";
        
        // Read the file and process the different instructions
        ifstream inputstream(this->filename);
        string line, currentInstruction;
        while (getline (inputstream, line)) {
            if (line.rfind(">>", 0) != 0) {
                // This line is a part of the previous instruction
                currentInstruction.append("\n");
                currentInstruction.append(line);
                continue;
            }
            // We have reached the next instruction, 
            // so process the previous instruction
            if (currentInstruction != "")   // Happens the first time
                processInstruction(currentInstruction);
            currentInstruction = line;
        }
        processInstruction(currentInstruction);
    }

}
