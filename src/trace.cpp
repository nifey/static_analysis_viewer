/*
 * Trace file parsing, traversal and rendering
 */
#include "trace.h"
#include "imnodes.h"
#include <iostream>
#include <fstream>
#include <gvc.h>

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

    bool operator!=(const Event& lhs, const Event& rhs) {
        if (lhs.type != rhs.type) return true;
        if (lhs.type == NODE_INFO)
            return tie(lhs.node1, lhs.tag, lhs.info) != tie(rhs.node1, rhs.tag, rhs.info);
        else if (lhs.type == EDGE_INFO)
            return tie(lhs.node1, lhs.node2, lhs.tag, lhs.info) != tie(rhs.node1, rhs.node2, rhs.tag, rhs.info);
        else if (lhs.type == GLOBAL_INFO)
            return tie(lhs.tag, lhs.info) != tie(rhs.tag, rhs.info);
        assert(false);
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

    string Graph::getNodeName(NodeID nodeID) {
        return nodeNames[nodeID];
    }

    string Graph::getNodeGroupName(NodeID nodeID) {
        auto splitName = splitOnFirst(nodeNames[nodeID], ":");
        return splitName.first;
    }

    string Graph::getNodeContents(NodeID nodeID) {
        return nodeContents[nodeID];
    }

    vector<NodeID> Graph::getActiveNodeIDs(string currentGroup) {
        vector<NodeID> activeNodes;
        for (auto entry: nodeIDs[currentGroup])
            activeNodes.push_back(entry.second);
        return activeNodes;
    }

    vector<pair<NodeID, NodeID>> Graph::getActiveEdges(string currentGroup) {
        vector<pair<NodeID,NodeID>> activeEdges;
        for (auto entry: nodeIDs[currentGroup])
            for (NodeID dstNodeID : edges[entry.second])
                activeEdges.push_back(make_pair(entry.second, dstNodeID));
        return activeEdges;
    }

    void Graph::renderGraphView(string currentGroup, Event currentEvent) {
        static AttributeID attrID = 0;
        ImNodes::BeginNodeEditor();
        for (NodeID nodeID : getActiveNodeIDs(currentGroup)) {
            ImNodes::BeginNode(nodeID);
            ImGui::Text(getNodeContents(nodeID).c_str());
            if (inputAttributeIDMap.find(nodeID) == inputAttributeIDMap.end())
                inputAttributeIDMap[nodeID] = attrID++;
            if (outputAttributeIDMap.find(nodeID) == outputAttributeIDMap.end())
                outputAttributeIDMap[nodeID] = attrID++;
            ImNodes::BeginInputAttribute(inputAttributeIDMap[nodeID]);
            ImNodes::EndInputAttribute();
            ImNodes::BeginOutputAttribute(outputAttributeIDMap[nodeID]);
            ImNodes::EndOutputAttribute();
            ImNodes::EndNode();
        }

        if (lastDisplayedGroup != currentGroup) {
            lastDisplayedGroup = currentGroup;

            GVC_t* gvc = gvContext();
            Agraph_t* G = agopen("graph", Agdirected, nullptr);
            agattr(G, AGNODE, "width", "1");
            agattr(G, AGNODE, "height", "1");
            agattr(G, AGNODE, "shape", "box");
            agattr(G, AGNODE, "fixedsize", "true");
            agset(G, "dpi", "72");
            const double dpi = 72.0;

            // Construct the Agraph from our graph representation
            map<NodeID, Agnode_t*> nodeMap;
            map<pair<NodeID,NodeID>, Agedge_t*> edgeMap;
            for (NodeID nodeID : getActiveNodeIDs(currentGroup))
                nodeMap[nodeID] = agnode(G, nullptr, true);
            for (auto edge : getActiveEdges(currentGroup))
                edgeMap[edge] = agedge(G, nodeMap[edge.first], nodeMap[edge.second], nullptr, true);
            for (NodeID nodeID : getActiveNodeIDs(currentGroup)) {
                auto dimensions = ImNodes::GetNodeDimensions(nodeID);
                agset(nodeMap[nodeID], "width", const_cast<char*>(to_string(dimensions[0]/dpi).c_str()));
                agset(nodeMap[nodeID], "height", const_cast<char*>(to_string(dimensions[1]/dpi).c_str()));
            }

            // Use GraphViz layout to layout the graph
            gvLayout(gvc, G, "dot");
            for (NodeID nodeID : getActiveNodeIDs(currentGroup)) {
                Agnode_t *anode = nodeMap[nodeID];
                auto pos = ND_coord(anode);
                auto width = ND_width(anode) * dpi;
                auto height = ND_height(anode) * dpi;
                ImNodes::SetNodeGridSpacePos(nodeID, ImVec2(pos.x + width, - height - pos.y));
                ImNodes::SetNodeDraggable(nodeID, true);
            }

            // Cleanup layout
            gvFreeLayout(gvc, G);
            agclose(G);
            gvFreeContext(gvc);
        }

        EdgeID edgeID = 0;
        map<AttributeID, map<AttributeID, EdgeID>> edgeIDMap;
        for (auto edge : getActiveEdges(currentGroup)) {
            AttributeID srcID = outputAttributeIDMap[edge.first];
            AttributeID dstID = inputAttributeIDMap[edge.second];
            if (edgeIDMap.find(srcID) == edgeIDMap.end())
                edgeIDMap[srcID];
            if (edgeIDMap[srcID].find(dstID) == edgeIDMap[srcID].end())
                edgeIDMap[srcID][dstID] = edgeID++;
            ImNodes::Link(edgeIDMap[srcID][dstID],
                    outputAttributeIDMap[edge.first],
                    inputAttributeIDMap[edge.second]);
        }

        if (lastDisplayedEvent != currentEvent) {
            lastDisplayedEvent = currentEvent;
            if (currentEvent.getType() == NODE_INFO) {
                NodeID currentNodeID = currentEvent.getNode1();
                ImNodes::ClearNodeSelection();
                ImNodes::ClearLinkSelection();
                auto pos = ImNodes::GetNodeGridSpacePos(currentNodeID);
                auto nodeSize = ImNodes::GetNodeDimensions(currentNodeID);
                auto editorSize = ImNodes::GetEditorDimensions();
                ImNodes::EditorContextResetPanning(ImVec2((editorSize.x - nodeSize.x)/ 2.0 - pos.x, (editorSize.y - nodeSize.y)/ 2.0 - pos.y));
                ImNodes::SelectNode(currentNodeID);
            } else if (currentEvent.getType() == EDGE_INFO) {
                NodeID currentNodeID1 = currentEvent.getNode1();
                NodeID currentNodeID2 = currentEvent.getNode2();
                ImNodes::ClearNodeSelection();
                ImNodes::ClearLinkSelection();
                auto pos1 = ImNodes::GetNodeGridSpacePos(currentNodeID1);
                auto pos2 = ImNodes::GetNodeGridSpacePos(currentNodeID2);
                auto pos =  ImVec2((pos1.x + pos2.x) / 2.0, (pos1.y + pos2.y) / 2.0);
                auto editorSize = ImNodes::GetEditorDimensions();
                ImNodes::EditorContextResetPanning(ImVec2(editorSize.x / 2.0 - pos.x, editorSize.y / 2.0 - pos.y));
                ImNodes::SelectLink(edgeIDMap[outputAttributeIDMap[currentNodeID1]][inputAttributeIDMap[currentNodeID2]]);
            }
        }
        ImNodes::MiniMap(0.3f, ImNodesMiniMapLocation_BottomRight);
        ImNodes::EndNodeEditor();
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
        } else if (instruction == ">>nodeinfo") {
            string tag = splitOnFirst(splitOnFirst(instructionHeader, " \t").second, " \t").second;
            Event event(EVENT_TYPE::NODE_INFO, instructionBody, tag);
            event.setNode(graph.getNodeID(headerTokens[1]));
            timeline.addEvent(event);
        } else if (instruction == ">>edgeinfo") {
            string tag = splitOnFirst(splitOnFirst(splitOnFirst(instructionHeader, " \t").second, " \t").second, " \t").second;
            Event event(EVENT_TYPE::EDGE_INFO, instructionBody, tag);
            event.setEdge(graph.getNodeID(headerTokens[1]), graph.getNodeID(headerTokens[2]));
            timeline.addEvent(event);
        } else if (instruction == ">>globalinfo") {
            string tag = splitOnFirst(instructionHeader, " \t").second;
            Event event(EVENT_TYPE::GLOBAL_INFO, instructionBody, tag);
            timeline.addEvent(event);
        } else if (instruction == ">>prevnodeinfo") {
            string tag = splitOnFirst(splitOnFirst(instructionHeader, " \t").second, " \t").second;
            string previnfo = timeline.getEventAtIndex(timeline.getCurrentPrevEventIndex()).getInfo();
            Event event(EVENT_TYPE::NODE_INFO, previnfo, tag);
            event.setNode(graph.getNodeID(headerTokens[1]));
            timeline.addEvent(event);
        } else if (instruction == ">>prevedgeinfo") {
            string tag = splitOnFirst(splitOnFirst(splitOnFirst(instructionHeader, " \t").second, " \t").second, " \t").second;
            string previnfo = timeline.getEventAtIndex(timeline.getCurrentPrevEventIndex()).getInfo();
            Event event(EVENT_TYPE::EDGE_INFO, previnfo, tag);
            event.setEdge(graph.getNodeID(headerTokens[1]), graph.getNodeID(headerTokens[2]));
            timeline.addEvent(event);
        } else if (instruction == ">>prevglobalinfo") {
            string tag = splitOnFirst(instructionHeader, " \t").second;
            string previnfo = timeline.getEventAtIndex(timeline.getCurrentPrevEventIndex()).getInfo();
            Event event(EVENT_TYPE::GLOBAL_INFO, previnfo, tag);
            timeline.addEvent(event);
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

    void Trace::render() {
        if (ImGui::IsKeyPressed(ImGuiKey_L, true) ||
                ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
            timeline.moveToNextEvent();
        if (ImGui::IsKeyPressed(ImGuiKey_H, true) ||
                ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
            timeline.moveToPrevEvent();
        if (ImGui::IsKeyPressed(ImGuiKey_K, true) ||
                ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))
            timeline.moveToCurrentNextEvent();
        if (ImGui::IsKeyPressed(ImGuiKey_J, true) ||
                ImGui::IsKeyPressed(ImGuiKey_DownArrow, true))
            timeline.moveToCurrentPrevEvent();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        auto screenSize = viewport->Size;
        double paneWidthRatio = 0.7;
        ImVec2 graphViewSize = ImVec2(screenSize.x * paneWidthRatio, screenSize.y);
        ImVec2 sidePaneSize = ImVec2(screenSize.x * (1.0 - paneWidthRatio), screenSize.y);

        auto windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | 
                ImGuiWindowFlags_NoDecoration;
        ImGui::Begin("Viewer", nullptr, windowFlags);

        // 1. Graph View
        ImGui::SetNextWindowSize(graphViewSize);
        graph.renderGraphView(timeline.getCurrentGroup(graph), timeline.getCurrentEvent());

        // 2. Side Pane
        ImGui::SameLine();
        ImGui::BeginChild("Side Pane", sidePaneSize, true, ImGuiChildFlags_FrameStyle);
        if (ImGui::TreeNodeEx("Info view", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
            if (timeline.size() != 0) {
                Event &currentEvent = timeline.getCurrentEvent();
                ImGui::Text(currentEvent.getTag().c_str());
                ImGui::Text(currentEvent.getInfo().c_str());
            }
            ImGui::TreePop();
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
