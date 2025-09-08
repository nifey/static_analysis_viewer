package com.omkar.LoggerLibrary;

import com.esotericsoftware.kryo.Kryo;

public class DfLogger{

    private Path logFilePath;

    private HashMap<String, Integer> nodesPerGroup;

    private final ExecutorService processingThreadPool;
    private final Thread writerThread;
    private final ConcurrentHashMap<Long, Future<String>> processedResults;
    private final AtomicLong sequenceNumber;
    private volatile boolean isRunning = true;
    private final ObjectPrinter objPrinter;

    public DfLogger(ObjectProcessor processor){
        DfLogger(null, processor);
    }

    public DfLogger(String path, ObjectPrinter objPrinter) {

        if(objPrinter == null){
            throw new IllegalArgumentException("ObjectPrinter cannot be null.");
        }

        this.objPrinter = objPrinter;

        // Determine the base directory
        Path baseDir;
        if (path == null || path.trim().isEmpty()) {
            // Use the current working directory of the application
            baseDir = Paths.get(".").toAbsolutePath();
        } else {
            baseDir = Paths.get(path);
        }

        // Ensure the parent directory exists before trying to create a file
        try {
            Files.createDirectories(baseDir);
        } catch (IOException e) {
            System.err.println("Failed to create parent directory: " + e.getMessage());
            this.logFilePath = null; // Set path to null to indicate failure
            return;
        }

        do {
            String uniqueFileName = "log_" + UUID.randomUUID().toString() + ".txt";
            tempLogFilePath = baseDir.resolve(uniqueFileName);
        } while (Files.exists(tempLogFilePath));
        

        this.logFilePath = tempLogFilePath;

        try {
            Files.createFile(this.logFilePath);
            System.out.println("New log file created at: " + this.logFilePath.toString());
        } catch (IOException e) {
            System.err.println("Failed to create log file: " + e.getMessage());
            this.logFilePath = null; 
        }

        this.nodesPerGroup = new HashMap<>();

        this.processedResults = new ConcurrentHashMap<>();
        this.sequenceNumber = new AtomicLong(0);

        // Creates a fixed thread pool to process events
        this.processingThreadPool = Executors.newFixedThreadPool(8); // Using 8 threads as requested
        
        this.writerThread = new Thread(this::writeLogQueue, "LogWriter-Thread");
        this.writerThread.setDaemon(true); 
        this.writerThread.start();
    }

    public String addNode(String nodeInfo){
        return addNode("default", nodeInfo);
    }

    public String addNode(String group, String nodeInfo){
        if(this.nodesPerGroup.containsKey(group)){
            this.nodesPerGroup.put(group, this.nodesPerGroup.get(group) + 1);
        }else{
            this.nodesPerGroup.put(group, 1);
        }
        String node = "n" + this.nodesPerGroup.get(group);
        addNode(group, node, nodeInfo);
        return node;
    }

    public void addNode(String group, String node, String nodeInfo){
        AddNodeInfoEvent event = new AddNodeInfoEvent(group, node, nodeInfo);
        log(event);
        return node; 
    }

    public void addEdge(String node1, String node2){
        addEdge("default", node1, "default", node2);
    }

    public void addEdge(String group1, String node1, String group2, String node2){
        AddEdgeInfoEvent event = new AddEdgeInfoEvent(group1, node1, group2, node2);
        log(event);
    }



    private String process(Event event){

    }

    public void log(Event event) {
        if (!isRunning) {
            System.err.println("Logger is shut down. Cannot log event.");
            return;
        }
        long currentSequence = sequenceNumber.getAndIncrement();
        Callable<String> processingTask = () -> {
            try {
                // The heavy lifting of processing the event happens here, in parallel
                return process(event);
            } catch (Exception e) {
                System.err.println("Error processing event: " + e.getMessage());
                return "Error processing event: " + e.getMessage();
            }
        };

        Future<String> future = processingThreadPool.submit(processingTask);
        processedResults.put(currentSequence, future);
    }

    private void writeLogQueue() {
        long currentSequence = 0;
        while (isRunning || !processedResults.isEmpty()) {
            Future<String> future = processedResults.get(currentSequence);
            if (future != null) {
                try {
                    String logEntry = future.get(); // Blocks until the processing is complete
                    String finalLogEntry = String.format("%s%n", logEntry);
                    Files.writeString(logFilePath, finalLogEntry, java.nio.file.StandardOpenOption.APPEND);
                    processedResults.remove(currentSequence);
                    currentSequence++;
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    System.err.println("Logging thread was interrupted while waiting for processing.");
                } catch (ExecutionException e) {
                    System.err.println("Error in processing task: " + e.getCause());
                    processedResults.remove(currentSequence);
                    currentSequence++;
                } catch (IOException e) {
                    System.err.println("Error writing to log file: " + e.getMessage());
                }
            } else {
                try {
                    TimeUnit.MILLISECONDS.sleep(100); // Wait a little if the next item isn't ready
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    System.err.println("Writer thread sleep interrupted.");
                }
            }
        }
        System.out.println("Log writer thread shut down.");
    }
    

    public void shutdown() {
        this.isRunning = false;
        processingThreadPool.shutdown();
        try {
            if (!processingThreadPool.awaitTermination(60, TimeUnit.SECONDS)) {
                processingThreadPool.shutdownNow();
            }
            this.writerThread.join();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }


}

class Event{
    
}

class AddNodeInfoEvent extends Event{
    String group;
    String node;
    String nodeInfo;
    NodeInfoEvent(String group, String node, String nodeInfo){
        this.group = group;
        this.node = node;
        this.nodeInfo = nodeInfo;
    }
}