# Static Analysis Viewer

![Screenshot](screenshot.png)

## Usage
```sh
# Pull libraries
git submodule update --init

# Install Dependencies
# On Linux
apt-get install libsdl2-dev libopengl-dev libgraphviz-dev
# On Mac
brew install sdl2

#To install graphviz on Mac 
brew install graphviz

#To verify the installation of graphviz use
dot -V

# Build
cd src
make

# Run
./sail_viewer <tracefile>
```

## Tracefile Format

The tracefile rendered by the viewer has to be in the following format. The tracefile consists of a series of instructions. Every instruction starts with a keyword beginning with a `>>` symbol at the beginning of a line. If the instruction takes a multi-line input argument then it will be delimited by the next instruction.

1. *Graph definition*: `>>node` and `>>edge` instructions are used to specify the nodes and edges of the graph to be viewed. Each node in the graph needs to be given a unique name or ID. Optionally, we can specify a group for the nodes to a certain portion of the graph at a time. For instance, in Inter-procedural analysis, the group names could be function names. The viewer would then render one function at a time.

    - To create a Graph node
        ```
        >>node [<groupname>:]<uniquenodeid>
        Data to be viewed in the node (eg: instructions)
        Can be multiple lines, Terminated by the next command in logfile
        ```

    - To create a Graph edge
        ```
        >>edge [<groupname>:]<srcnodeid> [<groupname>:]<dstnodeid>
        ```

2. *Sequence of Events*: The tracefile consists of a sequence of events that are logged. Each event would have some information (or Data flow facts), which can be associated with a node, or an edge, or can represent global information. The information of each event is a string (which can be multi-line). In addition to the information, we can also add an optional tag or comment that will be displayed in the viewer when the event is shown.

    - To create an event at a node:
        ```
        >>nodeinfo  [<groupname>:]<uniquenodeid> [<additional title or tag>]
        Data to be viewed in the node (eg: instructions)
        Can be multiple lines, Terminated by the next command in logfile
        ```
    - To create an event at an edge:
        ```
        >>edgeinfo [<groupname>:]<srcnodeid> [<groupname>:]<dstnodeid>  [<additional title or tag>]
        Data to be viewed in the node (eg: instructions)
        Can be multiple lines, Terminated by the next command in logfile
        ```
    - To create an event that is not associated with a node or edge:
        ```
        >>globalinfo [<additional title or tag>]
        Data to be viewed in the node (eg: instructions)
        Can be multiple lines, Terminated by the next command in logfile
        ```

3. *Optimizations to reduce tracefile size*: In order to prevent repeatedly printing the same information at different points in time, we have the `>>prev*info` instructions which would render the information present in a previous event that occurred at the same location (either node or edge or globally), and is of the same type. We can specify a different tag, if needed.
    ```
    >>prevnodeinfo  [<groupname>:]<uniquenodeid>  [<additional title or tag>]
    >>prevedgeinfo [<groupname>:]<srcnodeid> [<groupname>:]<dstnodeid>  [<additional title or tag>]
    >>prevglobalinfo [<additional title or tag>]
    ```
