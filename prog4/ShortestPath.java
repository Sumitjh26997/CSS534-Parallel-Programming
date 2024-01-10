import org.apache.spark.SparkConf;
import org.apache.spark.api.java.JavaPairRDD;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.api.java.JavaSparkContext;

import scala.Tuple2;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

public class ShortestPath {
	public static final String ACTIVE = "ACTIVE";
	public static final String INACTIVE = "INACTIVE";
	
	public static void main(String[] args) {
		// start Sparks and read a given input file
		String file = args[0];
		String source = args[1];
		String sink = args[2];
		
		SparkConf conf = new SparkConf().setAppName("Shortest Path BFS");
		JavaSparkContext sc = new JavaSparkContext(conf);
		JavaRDD<String> lines = sc.textFile(file);
		// now start a timer
		long startTime = System.currentTimeMillis();
		JavaPairRDD<String, Data> network =
				lines.mapToPair(line -> {
					int index = line.indexOf("=");
					String vertex = line.substring(0, index);
					
					String[] lists = line.substring(index + 1).split(";");
					int n = lists.length;
					
					List<Tuple2<String, Integer>> neighbors = new ArrayList<>();
					
					for (int i = 0; i < n; i++) {
						String[] parts = lists[i].split(",");
						neighbors.add(new Tuple2<>(parts[0], Integer.parseInt(parts[1])));
					}

					Data data_obj;
					if (vertex.equals(source)) {
						data_obj = new Data(neighbors, 0, 0, ACTIVE);
					} else {
						data_obj = new Data(neighbors, Integer.MAX_VALUE, Integer.MAX_VALUE, INACTIVE);
					}
					return new Tuple2<>(vertex, data_obj);
				});
		System.out.println("Count = " + network.count() + "\n");


		while (network.filter(f -> {
			if (f._2.status.equals(ACTIVE)) {
				return true;
			} else {
				return false;
			}
		}).count() > 0) {
			JavaPairRDD<String, Data> propagatedNetwork = network.flatMapToPair(vertex -> {
				// If a vertex is "ACTIVE", create Tuple2( neighbor, new Data() ) for
				// each neighbor where Data should include a new distance to this neighbor.
				// Add each Tuple2 to a list. Dont forget this vertex itself back to the
				// list. Return all the list items.
				List<Tuple2<String, Data>> list = new ArrayList<>();
				list.add(new Tuple2<>(vertex._1,
						new Data(vertex._2.neighbors, vertex._2.distance, vertex._2.prev,
								INACTIVE)));
				if (vertex._2.status.equals(ACTIVE)) {
					for (Tuple2<String, Integer> neighbor : vertex._2.neighbors) {
						list.add(new Tuple2<>(neighbor._1,
								new Data(new ArrayList<>(), (neighbor._2 + vertex._2.distance),
										Integer.MAX_VALUE, INACTIVE)));
					}
				}
				return list.iterator();
			});
			
			network = propagatedNetwork.reduceByKey((k1, k2) -> {
				// For each key, (i.e, each vertex), find the shortest distance and
				// update this vertex Data attribute.
				List<Tuple2<String, Integer>> neighbors =
						k1.neighbors.size() == 0 ? k2.neighbors : k1.neighbors;
				int distance = Math.min(k1.distance, k2.distance);
				int prev = Math.min(k1.prev, k2.prev);
				return new Data(neighbors, distance, prev, INACTIVE);
			});
			
			network = network.mapValues(value -> {
				// If a vertex new distance is shorter than prev, activate this vertex
				// status and replace prev with the new distance.
				if (value.distance < value.prev) {
					return new Data(value.neighbors, value.distance, value.distance, ACTIVE);
				}
				return value;
			});
		}
		
			List<Data> res = network.lookup(sink);
		
			System.out.println("from " + source + " to " + sink + " takes distance = " + res.get(0).distance);
		
			System.out.println("Time = " + (System.currentTimeMillis() - startTime));
	}
}