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

    enum EVENT_TYPE { NODE_INFO, EDGE_INFO, GLOBAL_INFO };
    typedef tuple<EVENT_TYPE, string, unsigned long long, NodeID, NodeID> Event;

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
            map<pair<AttributeID, AttributeID>, EdgeID> attrToLinkIDMap;
            map<EdgeID, pair<NodeID, NodeID>> linkIDToNodeIDMap;

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
            pair<NodeID, NodeID> getLink(EdgeID linkID);

            vector<NodeID> getActiveNodeIDs(string currentGroup);
            vector<pair<NodeID, NodeID>> getActiveEdges(string currentGroup);

            // Render the active nodes in the NodeEditor
            void renderGraphView(string currentGroup, Event currentEvent);
    };

    class Timeline {
        private:
            // Vector of info strings
            vector<string> eventInfoStrings;
            // Event : Type, Tag, InfoStrIndex, NodeID1, NodeID2
            vector<tuple<EVENT_TYPE, string, unsigned long long, NodeID, NodeID>> eventList;
            // Map from the Event tuple to a sorted vector indices in the eventList
            // Used for moving between events of the same node / edge, and for hovering
            map<tuple<enum EVENT_TYPE, NodeID, NodeID>, vector<unsigned long long>> eventData;

            unsigned long long currentTimelineIndex = 0;
            float timelineFloatPosition = 0.0;

        public:
            void addEvent(EVENT_TYPE type, string tag, string info, NodeID node1, NodeID node2);
            void addEvent(EVENT_TYPE type, string tag, unsigned long long prevInfoStrIndex, NodeID node1, NodeID node2);
            unsigned long long size()   { return eventList.size(); }
            Event& getCurrentEvent()    { return eventList[currentTimelineIndex]; }
            Event& getEventAtIndex(unsigned long long index)    { return eventList[index]; }
            string& getStringAtIndex(unsigned long long index)    { return eventInfoStrings[index]; }

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

            unsigned long long getIndexInSortedList(vector<unsigned long long> &list, unsigned long long searchValue) {
                if (list.size() == 1) return 0;
                unsigned long long minIndex = 0, middleIndex = 0;
                unsigned long long maxIndex = list.size() - 1;
                while (minIndex < maxIndex) {
                    middleIndex = (minIndex + maxIndex) / 2;
                    if (list[middleIndex] == searchValue)
                        return middleIndex;
                    else if (list[middleIndex] > searchValue)
                        maxIndex = middleIndex - 1;
                    else
                        minIndex = middleIndex + 1;
                }
                return 0;
            }

            unsigned long long getPrevEventIndex(unsigned long long currentIndex, EVENT_TYPE type,
                    NodeID node1 = 0, NodeID node2 = 0) {
                auto eventTuple = make_tuple(type, node1, node2);
                auto size = eventData[eventTuple].size();
                if (size == 0) return currentTimelineIndex;

                auto eventDataIndex = getIndexInSortedList(eventData[eventTuple], currentTimelineIndex);
                if (eventDataIndex == 0)
                    return eventData[eventTuple][0];
                return eventData[eventTuple][eventDataIndex - 1];
            }
            unsigned long long getNextEventIndex(unsigned long long currentIndex, EVENT_TYPE type,
                    NodeID node1 = 0, NodeID node2 = 0) {
                auto eventTuple = make_tuple(type, node1, node2);
                auto size = eventData[eventTuple].size();
                if (size == 0) return currentTimelineIndex;

                auto eventDataIndex = getIndexInSortedList(eventData[eventTuple], currentTimelineIndex);
                if (eventDataIndex == size - 1)
                    return eventData[eventTuple][size - 1];
                return eventData[eventTuple][eventDataIndex + 1];
            }
            unsigned long long getCurrentPrevEventIndex() {
                EVENT_TYPE type; NodeID node1, node2;
                tie(type, ignore, ignore, node1, node2) = eventList[currentTimelineIndex];
                return getPrevEventIndex(currentTimelineIndex, type, node1, node2);
            }
            unsigned long long getCurrentNextEventIndex() {
                EVENT_TYPE type; NodeID node1, node2;
                tie(type, ignore, ignore, node1, node2) = eventList[currentTimelineIndex];
                return getNextEventIndex(currentTimelineIndex, type, node1, node2);
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
                while (lastGraphEventIndex > 0 && get<0>(eventList[lastGraphEventIndex]) == GLOBAL_INFO)
                    lastGraphEventIndex--;
                if (get<0>(eventList[lastGraphEventIndex]) == GLOBAL_INFO)
                    return "";
                NodeID lastActiveNode = get<3>(eventList[lastGraphEventIndex]);
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
