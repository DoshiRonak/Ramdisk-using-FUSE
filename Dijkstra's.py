def dijkstra(graph,source):
    unvisited = []
    dist = {}
    dists= {}
    prev = {}

    for v in graph:
        dist[v] = float('inf')
        dists[v] = float('inf')
        prev[v] = []
        unvisited.append(v)

    dist[source] = 0
    dists[source] = 0
    
    while(unvisited):
        u = min(dists, key=dists.get)
        if (u in unvisited):
            unvisited.remove(u)            
        
        for neighbour in graph[u]:
            if neighbour in unvisited:

                newdist = dist[u] + graph[u][neighbour]
                if newdist < dist[neighbour]:
                    dist[neighbour] = newdist
                    dists[neighbour] = newdist
                    prev[neighbour] = u
        
        del dists[u]

    for i in dist:
        dist[i] = [dist[i],prev[i]]

    return dist


if __name__ == "__main__":

    dict2 = {}
    nodes = []
    patharray = []
    try:
        file1=open('graph.txt','r')
        for line in file1:
            dict1 = {}
            line=line.strip('\n')
            keyvalue=line.split()
            key = keyvalue[0]
            nodes.append(keyvalue[0])
            value = keyvalue[1]
            keyvalue1 = value.split(',')
            k = keyvalue1[::2]
            v = keyvalue1[1::2]
            for i in range(len(k)):
                dict1[k[i]] = int(v[i])
            
            dict2[key] = dict1
        file1.close()
            
    except:
        file1.close()

    graph = dict2
    for i in range(len(nodes)):
        shortestpath = dijkstra(graph,nodes[i])
        print 'Node ' + nodes[i] + ': ', shortestpath
        patharray.append(shortestpath)  
    print ''    
    #print patharray

    while(1):
        source = str(raw_input('Enter the source node: '))
        dest = str(raw_input('Enter the destination node: '))
        path = []
        shortestpath = dijkstra(graph,source)
        path.insert(0,dest)
        a = shortestpath[dest]
        cost = a[0]
        while (str(a[1]) != source):
            path.insert(0,a[1])
            a = shortestpath[str(a[1])]

        path.insert(0,source)            

        print 'The Path from source to destination will be = ' + str(path) + ' and cost = ' + str(cost)
