/*
 * Trace file parsing, traversal and rendering
 */
#include "trace.h"
#include "imnodes.h"
#include <iostream>
#include <fstream>
#include <regex>
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

    pair<NodeID, NodeID> Graph::getLink(EdgeID linkID) {
        return linkIDToNodeIDMap[linkID];
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

        for (auto edge : getActiveEdges(currentGroup)) {
            AttributeID srcID = outputAttributeIDMap[edge.first];
            AttributeID dstID = inputAttributeIDMap[edge.second];
            pair<AttributeID, AttributeID> attrEdge = make_pair(srcID, dstID);
            if (attrToLinkIDMap.find(attrEdge) == attrToLinkIDMap.end()) {
                EdgeID newID = attrToLinkIDMap.size();
                attrToLinkIDMap[attrEdge] = newID;
                linkIDToNodeIDMap[newID] = make_pair(edge.first, edge.second);
            }
            ImNodes::Link(attrToLinkIDMap[attrEdge],
                    outputAttributeIDMap[edge.first],
                    inputAttributeIDMap[edge.second]);
        }

        if (lastDisplayedEvent != currentEvent) {
            lastDisplayedEvent = currentEvent;
            if (get<0>(currentEvent) == NODE_INFO) {
                NodeID currentNodeID = get<3>(currentEvent);
                ImNodes::ClearNodeSelection();
                ImNodes::ClearLinkSelection();
                auto pos = ImNodes::GetNodeGridSpacePos(currentNodeID);
                auto nodeSize = ImNodes::GetNodeDimensions(currentNodeID);
                auto editorSize = ImNodes::GetEditorDimensions();
                ImNodes::EditorContextResetPanning(ImVec2((editorSize.x - nodeSize.x)/ 2.0 - pos.x, (editorSize.y - nodeSize.y)/ 2.0 - pos.y));
                ImNodes::SelectNode(currentNodeID);
            } else if (get<0>(currentEvent) == EDGE_INFO) {
                NodeID currentNodeID1 = get<3>(currentEvent);
                NodeID currentNodeID2 = get<4>(currentEvent);
                ImNodes::ClearNodeSelection();
                ImNodes::ClearLinkSelection();
                auto pos1 = ImNodes::GetNodeGridSpacePos(currentNodeID1);
                auto pos2 = ImNodes::GetNodeGridSpacePos(currentNodeID2);
                auto pos =  ImVec2((pos1.x + pos2.x) / 2.0, (pos1.y + pos2.y) / 2.0);
                auto editorSize = ImNodes::GetEditorDimensions();
                ImNodes::EditorContextResetPanning(ImVec2(editorSize.x / 2.0 - pos.x, editorSize.y / 2.0 - pos.y));
                ImNodes::SelectLink(attrToLinkIDMap[make_pair(
                                outputAttributeIDMap[currentNodeID1],
                                inputAttributeIDMap[currentNodeID2])]);
            }
        }
        ImNodes::MiniMap(0.3f, ImNodesMiniMapLocation_BottomRight);
        ImNodes::EndNodeEditor();
    }

    void Timeline::addEvent(EVENT_TYPE type, string tag, string infoStr,
            NodeID node1 = 0, NodeID node2 = 0) {
        unsigned long long currentGlobalTimelineIndex = eventList.size();
        unsigned long long infoStrIndex = eventInfoStrings.size();
        eventInfoStrings.push_back(infoStr);
        eventList.push_back(make_tuple(type, tag, infoStrIndex, node1, node2));

        auto eventTuple = make_tuple(type, node1, node2);
        eventData[eventTuple].push_back(currentGlobalTimelineIndex);
    }

    void Timeline::addEvent(EVENT_TYPE type, string tag, unsigned long long prevInfoStrIndex,
            NodeID node1 = 0, NodeID node2 = 0) {
        unsigned long long currentGlobalTimelineIndex = eventList.size();
        eventList.push_back(make_tuple(type, tag, prevInfoStrIndex, node1, node2));

        auto eventTuple = make_tuple(type, node1, node2);
        eventData[eventTuple].push_back(currentGlobalTimelineIndex);
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
            timeline.addEvent(NODE_INFO, instructionBody, tag, graph.getNodeID(headerTokens[1]));
        } else if (instruction == ">>edgeinfo") {
            string tag = splitOnFirst(splitOnFirst(splitOnFirst(instructionHeader, " \t").second, " \t").second, " \t").second;
            timeline.addEvent(EDGE_INFO, instructionBody, tag, graph.getNodeID(headerTokens[1]), graph.getNodeID(headerTokens[2]));
        } else if (instruction == ">>globalinfo") {
            string tag = splitOnFirst(instructionHeader, " \t").second;
            timeline.addEvent(EVENT_TYPE::GLOBAL_INFO, instructionBody, tag);
        } else if (instruction == ">>prevnodeinfo") {
            string tag = splitOnFirst(splitOnFirst(instructionHeader, " \t").second, " \t").second;
            NodeID node1 = graph.getNodeID(headerTokens[1]);
            unsigned long long prevInfoStrIndex = get<2>(timeline.getEventAtIndex(
                        timeline.getPrevEventIndex(timeline.size()-1, NODE_INFO, node1)));
            timeline.addEvent(NODE_INFO, tag, prevInfoStrIndex, node1);
        } else if (instruction == ">>prevedgeinfo") {
            string tag = splitOnFirst(splitOnFirst(splitOnFirst(instructionHeader, " \t").second, " \t").second, " \t").second;
            NodeID node1 = graph.getNodeID(headerTokens[1]), node2 = graph.getNodeID(headerTokens[2]);
            unsigned long long prevInfoStrIndex = get<2>(timeline.getEventAtIndex(
                        timeline.getPrevEventIndex(timeline.size()-1, EDGE_INFO, node1, node2)));
            timeline.addEvent(EDGE_INFO, tag, prevInfoStrIndex, node1, node2);
        } else if (instruction == ">>prevglobalinfo") {
            string tag = splitOnFirst(instructionHeader, " \t").second;
            unsigned long long prevInfoStrIndex = get<2>(timeline.getEventAtIndex(
                        timeline.getPrevEventIndex(timeline.size()-1, GLOBAL_INFO)));
            timeline.addEvent(GLOBAL_INFO, tag, prevInfoStrIndex);
        } else {
            cout << "Unknown instruction " << instruction << " found in " << instructionHeader << "\n";
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

    float timelinePos = 0.0;
    char regexString[100];
    void Trace::render() {
        if (timelinePos != timeline.getFloatPosition())
            timeline.moveToFloatPosition(timelinePos);

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

        timelinePos = timeline.getFloatPosition();

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
        if (timeline.size() == 0) return;
        ImGui::BeginChild("Side Pane", sidePaneSize, true, ImGuiChildFlags_FrameStyle);
        if (ImGui::ArrowButton("Left", ImGuiDir_Left))
            timeline.moveToPrevEvent();
        ImGui::SameLine();
        if (ImGui::ArrowButton("Right", ImGuiDir_Right))
            timeline.moveToNextEvent();
        ImGui::SameLine();
        ImGui::SliderFloat("Timeline", &timelinePos, 0.0, 1.0, "", 0);

        Event &currentEvent = timeline.getCurrentEvent();
        string currentEventTag = get<1>(currentEvent);
        string currentEventInfo = timeline.getStringAtIndex(get<2>(currentEvent));

        // If any node or edge is being hovered, display the Prev Info at that node/edge
        int hoveredID;
        auto currentTimelineIndex = timeline.getTimelineIndex();
        if (ImNodes::IsNodeHovered(&hoveredID)) {
            unsigned long long prevNodeIndex = timeline.getPrevEventIndex(
                    currentTimelineIndex, NODE_INFO, hoveredID);
            if (prevNodeIndex > currentTimelineIndex) {
                currentEventTag = "";
                currentEventInfo = "";
            } else {
                Event &prevNodeEvent = timeline.getEventAtIndex(prevNodeIndex);
                currentEventTag = get<1>(prevNodeEvent);
                currentEventInfo = timeline.getStringAtIndex(get<2>(prevNodeEvent));
            }
        }
        if (ImNodes::IsLinkHovered(&hoveredID)) {
            NodeID node1, node2;
            std::tie (node1, node2) = graph.getLink(hoveredID);
            unsigned long long prevNodeIndex = timeline.getPrevEventIndex(
                    currentTimelineIndex, EDGE_INFO, node1, node2);
            if (prevNodeIndex > currentTimelineIndex) {
                currentEventTag = "";
                currentEventInfo = "";
            } else {
                Event &prevNodeEvent = timeline.getEventAtIndex(prevNodeIndex);
                currentEventTag = get<1>(prevNodeEvent);
                currentEventInfo = timeline.getStringAtIndex(get<2>(prevNodeEvent));
            }
        }

        if (ImGui::TreeNodeEx("Info view", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
            ImGui::Text(currentEventTag.c_str());
            ImGui::Text(currentEventInfo.c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Filtered Info view", ImGuiTreeNodeFlags_SpanFullWidth)) {
            ImGui::InputText("Regular Expression", regexString, 99);
            regex regex(regexString, regex_constants::basic | regex_constants::icase);
            string stringToDisplay;
            smatch match;
            for (string line : splitOn(currentEventInfo, "\n"))
                if (regex_search(line, match, regex))
                    stringToDisplay.append(line + "\n");
            ImGui::Text(stringToDisplay.c_str());
            ImGui::TreePop();
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
