from slither.slither import Slither

# -------------------
# Trace Logger Class
# -------------------
class TraceLogger:
    def __init__(self, filename):
        self.file = open(filename, "w")
        self.last_nodeinfo = {}

    def close(self):
        self.file.close()

    def selectGroup(self, name):
        self.file.write(f"selectgroup {name}\n")

    def addNode(self, group, nodeid, content=None):
        self.file.write(f"node {group}:{nodeid}\n")
        if content:
            self.file.write(content.replace("\n", " ") + "\n")

    def addEdge(self, group, src, dst):
        self.file.write(f"edge {group}:{src} {group}:{dst}\n")

    def addNodeInfo(self, group, nodeid, facts, tag=""):
        key = (group, nodeid, tag)
        if key in self.last_nodeinfo and self.last_nodeinfo[key] == facts:
            self.file.write(f"prevnodeinfo {group}:{nodeid} {tag}\n")
        else:
            self.file.write(f"nodeinfo {group}:{nodeid} {tag}\n")
            self.file.write(facts + "\n")
            self.last_nodeinfo[key] = facts


# -------------------
# Analysis Function
# -------------------
def analyze_contract(filename, tracefile):
    sl = Slither(filename)
    logger = TraceLogger(tracefile)

    for contract in sl.contracts:
        for function in contract.functions:
            if function.is_implemented:
                group = function.name
                logger.selectGroup(group)

                # Add nodes
                for node in function.nodes:
                    nodeid = f"BB{node.node_id}"
                    logger.addNode(group, nodeid, str(node))

                    # Fake Apron facts for now
                    fake_facts = f'{{"var_intervals": {{"amount": "[0,100]"}}}}'
                    logger.addNodeInfo(group, nodeid, fake_facts)

                # Add edges
                for node in function.nodes:
                    for succ in node.sons:
                        logger.addEdge(group, f"BB{node.node_id}", f"BB{succ.node_id}")

    logger.close()
    print(f"Trace written to {tracefile}")


# -------------------
# Entry Point
# -------------------
if __name__ == "__main__":
    analyze_contract("sample.sol", "analysis.trace")
