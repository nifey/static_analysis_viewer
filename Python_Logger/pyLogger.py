from typing import Optional, TextIO

class DfLogger:
    def __init__(self, filepath: str):
        """
        Initialize the logger with a file to write trace instructions.
        
        Args:
            filepath (str): Path to the tracefile to write.
        """
        self.filepath = filepath
        self.file: Optional[TextIO] = None

    def __enter__(self):
        """Enable use as a context manager."""
        self.file = open(self.filepath, 'w')
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Close the file when done."""
        if self.file:
            self.file.close()

    def _write_instruction(self, instruction: str, *args, content: Optional[str] = None):
        """
        Generic method to write an instruction to the tracefile.
        
        Args:
            instruction (str): Instruction keyword (without >>)
            args (tuple): Arguments for the instruction
            content (str, optional): Optional multi-line content
        """
        line = f">>{instruction}"
        if args:
            line += " " + " ".join(args)
        self.file.write(line + "\n")
        if content:
            self.file.write(content.rstrip() + "\n")  # Avoid extra trailing spaces

    # ------------------------
    # Graph structure methods
    # ------------------------
    def addNode(self, nodeid: str, group: Optional[str] = None, content: Optional[str] = None):
        """
        Define a node.
        
        Args:
            nodeid (str): Unique node identifier
            group (str, optional): Optional group name
            content (str, optional): Content to display in the node
        """
        nid = f"{group}:{nodeid}" if group else nodeid
        self._write_instruction("node", nid, content=content)

    def addEdge(self, src: str, dst: str, src_group: Optional[str] = None, dst_group: Optional[str] = None):
        """
        Define an edge between nodes.
        
        Args:
            src (str): Source node id
            dst (str): Destination node id
            src_group (str, optional): Group name of source node
            dst_group (str, optional): Group name of destination node
        """
        srcid = f"{src_group}:{src}" if src_group else src
        dstid = f"{dst_group}:{dst}" if dst_group else dst
        self._write_instruction("edge", srcid, dstid)

    # ------------------------
    # Node/Edge/Global info
    # ------------------------
    def addNodeInfo(self, nodeid: str, data: str, group: Optional[str] = None, tag: Optional[str] = None):
        """
        Add dataflow info for a node.
        """
        nid = f"{group}:{nodeid}" if group else nodeid
        args = [nid]
        if tag:
            args.append(tag)
        self._write_instruction("nodeinfo", *args, content=data)

    def addEdgeInfo(self, src: str, dst: str, data: str, src_group: Optional[str] = None, dst_group: Optional[str] = None, tag: Optional[str] = None):
        """
        Add dataflow info for an edge.
        """
        srcid = f"{src_group}:{src}" if src_group else src
        dstid = f"{dst_group}:{dst}" if dst_group else dst
        args = [srcid, dstid]
        if tag:
            args.append(tag)
        self._write_instruction("edgeinfo", *args, content=data)

    def addGlobalInfo(self, data: str, tag: Optional[str] = None):
        """Add global dataflow info."""
        args = [tag] if tag else []
        self._write_instruction("globalinfo", *args, content=data)

    # ------------------------
    # Reusing previous info
    # ------------------------
    def addPrevNodeInfo(self, nodeid: str, group: Optional[str] = None, tag: Optional[str] = None):
        """Reference previously logged node info."""
        nid = f"{group}:{nodeid}" if group else nodeid
        args = [nid]
        if tag:
            args.append(tag)
        self._write_instruction("prevnodeinfo", *args)

    def addPrevEdgeInfo(self, src: str, dst: str, src_group: Optional[str] = None, dst_group: Optional[str] = None, tag: Optional[str] = None):
        """Reference previously logged edge info."""
        srcid = f"{src_group}:{src}" if src_group else src
        dstid = f"{dst_group}:{dst}" if dst_group else dst
        args = [srcid, dstid]
        if tag:
            args.append(tag)
        self._write_instruction("prevedgeinfo", *args)

    def addPrevGlobalInfo(self, tag: Optional[str] = None):
        """Reference previously logged global info."""
        args = [tag] if tag else []
        self._write_instruction("prevglobalinfo", *args)

    # ------------------------
    # Group selection
    # ------------------------
    def selectGroup(self, group: str):
        """Switch to viewing a new group."""
        self._write_instruction("selectgroup", group)


# ------------------------
# Example usage
# ------------------------
if __name__ == "__main__":
    with DfLogger("tracefile.log") as logger:
        logger.selectGroup("functionA")
        logger.addNode("n1", content="x = 1")
        logger.addNode("n2", content="y = x + 2")
        logger.addEdge("n1", "n2")
        logger.addNodeInfo("n1", '{"defined_vars": ["x"]}')
        logger.addNodeInfo("n2", '{"used_vars": ["x"], "defined_vars": ["y"]}')
        logger.addPrevNodeInfo("n1")
        logger.addGlobalInfo('{"heap": {"x": 1, "y": 3}}')
