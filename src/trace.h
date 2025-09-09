/*
 * Trace class contians the trace file parsing, traversal and rendering
 */
#include <map>
#include <set>
#include <vector>
#include <string>
#include <SDL.h>

using namespace std;

namespace sail {

    typedef unsigned long long NodeID;
    typedef unsigned long long EdgeID;
    typedef unsigned long long AttributeID;

    enum EVENT_TYPE {
        NODE_INFO,
        EDGE_INFO,
        GLOBAL_INFO,
    };

    class Event {
        private:
            EVENT_TYPE type;
            string tag, info;

            // Depending on the type of event, we will use zero (global),
            // one (node) or two(edge) Node IDs below
            NodeID node1, node2;

        public:
            Event () {};
            Event (EVENT_TYPE type, string info, string tag) 
                : type(type), info(info), tag(tag) {};
            void setNode(NodeID nodeID) { node1 = nodeID; }
            void setEdge(NodeID srcNodeID, NodeID dstNodeID) {
                node1 = srcNodeID; node2 = dstNodeID;
            }

            // Other getters
            EVENT_TYPE getType() { return type; }
            string getTag() { return tag; }
            string getInfo() { return info; }
            NodeID getNode1() { return node1; }
            NodeID getNode2() { return node2; }

            friend bool operator!=(const Event& lhs, const Event& rhs);
    };

    class Graph {
        private:
            // Maps groupName -> Map of node names in that group to the corresponding NodeID
            map<string,map<string,NodeID>> nodeIDs;
            // Vector of node names. The NodeID will be the index into this vector
            vector<string> nodeNames;
            // Node contents
            map<NodeID,string> nodeContents;

            // Edges of the graph in adjancency list format
            // Map from source Node ID to vector of destination Node ID
            map<NodeID, set<NodeID>> edges;

            // Map from nodeID to input/output attribute ID
            map<NodeID, AttributeID> inputAttributeIDMap;
            map<NodeID, AttributeID> outputAttributeIDMap;

            // Last displayed group : Used to figure out when to call GraphViz for layout
            string lastDisplayedGroup = "-------";
            Event lastDisplayedEvent;

        public:
            void addNode(string nodeName, std::string nodeContents); 
            void addEdge(string srcNodeName, string dstNodeName); 
            NodeID getNodeID(string nodeName);
            std::string getNodeName(NodeID nodeID);
            std::string getNodeGroupName(NodeID nodeID);
            std::string getNodeContents(NodeID nodeID);

            vector<NodeID> getActiveNodeIDs(string currentGroup);
            vector<pair<NodeID, NodeID>> getActiveEdges(string currentGroup);

            // Render the active nodes in the NodeEditor
            void renderGraphView(string currentGroup, Event currentEvent);
    };

    class Timeline {
        private:
            vector<Event> eventList;
            unsigned long long currentTimelineIndex = 0;
            float timelineFloatPosition = 0.0;
        public:
            void addEvent(Event event)  { eventList.push_back(event); }
            unsigned long long size()   { return eventList.size(); }
            Event& getCurrentEvent()    { return eventList[currentTimelineIndex]; }
            Event& getEventAtIndex(unsigned long long index)    { return eventList[index]; }

            void setTimelineIndex(unsigned long long index) {
                currentTimelineIndex = index;
                timelineFloatPosition = ((float) currentTimelineIndex / eventList.size());
            }
            unsigned long long getTimelineIndex() {
                return currentTimelineIndex;
            }
            void setFloatPosition(float position) {
                currentTimelineIndex = position * eventList.size();
                timelineFloatPosition = ((float) currentTimelineIndex / eventList.size());
            }
            float getFloatPosition () {
                return ((float) currentTimelineIndex) / eventList.size();
            }

            void moveToNextEvent() {
                if (currentTimelineIndex < eventList.size() - 1)
                    setTimelineIndex(currentTimelineIndex+1);
            }
            void moveToPrevEvent() {
                if (currentTimelineIndex > 0)
                    setTimelineIndex(currentTimelineIndex-1);
            }
            unsigned long long getCurrentPrevEventIndex() {
                Event &currentEvent = eventList[currentTimelineIndex];
                EVENT_TYPE type = currentEvent.getType();
                if (currentTimelineIndex == 0) return currentTimelineIndex;
                for (unsigned long long index = currentTimelineIndex - 1; index >= 0; index--) {
                    if (eventList[index].getType() == type) {
                        if (type == GLOBAL_INFO) return index;
                        else if (eventList[index].getNode1() == currentEvent.getNode1()) {
                            if (type == NODE_INFO) return index;
                            else if (eventList[index].getNode2() == currentEvent.getNode2()) {
                                if (type == EDGE_INFO) return index;
                            }
                        }
                    }
                    if (index == 0) break;
                }
                return currentTimelineIndex;
            }
            unsigned long long getCurrentNextEventIndex() {
                Event &currentEvent = eventList[currentTimelineIndex];
                EVENT_TYPE type = currentEvent.getType();
                unsigned long long endIndex = eventList.size() - 1;
                if (currentTimelineIndex == endIndex) return currentTimelineIndex;
                for (unsigned long long index = currentTimelineIndex + 1; index <= endIndex; index++) {
                    if (eventList[index].getType() == type) {
                        if (type == GLOBAL_INFO) return index;
                        else if (eventList[index].getNode1() == currentEvent.getNode1()) {
                            if (type == NODE_INFO) return index;
                            else if (eventList[index].getNode2() == currentEvent.getNode2()) {
                                if (type == EDGE_INFO) return index;
                            }
                        }
                    }
                }
                return currentTimelineIndex;
            }
            void moveToCurrentNextEvent() {
                setTimelineIndex(getCurrentNextEventIndex());
            }
            void moveToCurrentPrevEvent() {
                setTimelineIndex(getCurrentPrevEventIndex());
            }
            void moveToFloatPosition (float timelinePosition) {
                currentTimelineIndex = timelinePosition * eventList.size();
            }

            string getCurrentGroup(Graph &graph) {
                unsigned long long lastGraphEventIndex = currentTimelineIndex;
                if (eventList.size() == 0) return "";
                while (lastGraphEventIndex > 0 && eventList[lastGraphEventIndex].getType() == GLOBAL_INFO)
                    lastGraphEventIndex--;
                if (eventList[lastGraphEventIndex].getType() == GLOBAL_INFO)
                    return "";
                NodeID lastActiveNode = eventList[lastGraphEventIndex].getNode1();
                return graph.getNodeGroupName(lastActiveNode);
            }
    };

    class Trace {
        private:
            // Filename of the trace
            string filename;
            // Graph representation
            Graph graph;
            // Timeline
            Timeline timeline;
            
            void processInstruction(string currentInstruction);

        public:
            // Constructor which reads file 
            // and parses the trace
            Trace(string _filename);

            // Main Render function
            void render();
    };

}
