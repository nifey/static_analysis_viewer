/*
 * Trace class contians the trace file parsing, traversal and rendering
 */
#include <map>
#include <set>
#include <vector>
#include <string>

using namespace std;

namespace sail {

    typedef unsigned long long NodeID;
    typedef unsigned long long EdgeID;
    typedef unsigned long long AttributeID;

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

        public:
            void addNode(string nodeName, std::string nodeContents); 
            void addEdge(string srcNodeName, string dstNodeName); 
            NodeID getNodeID(string nodeName);
            std::string getNodeName(NodeID nodeID);
            std::string getNodeContents(NodeID nodeID);

            vector<NodeID> getActiveNodeIDs();
            vector<pair<NodeID, NodeID>> getActiveEdges();

            // Render the active nodes in the NodeEditor
            void renderGraphView();
    };

    class Trace {
        private:
            // Filename of the trace
            string filename;
            // Graph representation
            Graph graph;
            
            void processInstruction(string currentInstruction);

        public:
            // Constructor which reads file 
            // and parses the trace
            Trace(string _filename);

            // Main Render function
            void render();
    };

}
