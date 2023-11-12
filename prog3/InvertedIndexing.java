import java.io.IOException;
import java.util.*;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapred.*;
import org.apache.hadoop.util.*;

public class InvertedIndexing {

	public static class Map extends MapReduceBase implements Mapper<LongWritable, Text, Text, Text> {
		JobConf conf;
		public void configure( JobConf job ) {
			this.conf = job;
			}
			public void map(LongWritable docId, Text value, OutputCollector<Text, Text> output,
			Reporter reporter) throws IOException {
				// retrieve # keywords from JobConf
				int argc = Integer.parseInt( conf.get( "argc" ) );
				// get the current file name
				Set<String> args = new HashSet();
				// retrieve keywords
				for (int i = 0; i < argc; i++) {
					args.add(conf.get("keyword" + i));
				}

				FileSplit fileSplit = ( FileSplit )reporter.getInputSplit( );
				String filename = "" + fileSplit.getPath( ).getName( );
				String line = value.toString();
	    	StringTokenizer tokenizer = new StringTokenizer(line);
				//collect if next token match one of the args
				while (tokenizer.hasMoreTokens()) {
					String token = tokenizer.nextToken();
					if (args.contains(token)) {
						output.collect(new Text(token), new Text(filename));
					}
				}
		}
	}

	public static class Reduce extends MapReduceBase implements Reducer<Text, Text, Text, Text> {
		public void reduce(Text key, Iterator<Text> values, OutputCollector<Text, Text> output,
		Reporter reporter) throws IOException {
			HashMap<String, Integer> hm = new HashMap<String, Integer>();
			//Count the occurrence number of key in each file
			while (values.hasNext()) {
				String name = values.next().toString();
				if (hm.containsKey(name)) {
					hm.put(name, hm.get(name) + 1);
				} else {
					hm.put(name, 1);
				}
			}
			//create Comparator to sort the result by count number
			Comparator<java.util.Map.Entry<String, Integer>> valueComparator =
					new Comparator<java.util.Map.Entry<String, Integer>>() {
						@Override
						public int compare(java.util.Map.Entry<String, Integer> e1, java.util.Map.Entry<String, Integer> e2) {
							int v1 = e1.getValue();
							int v2 = e2.getValue();
							return v1 - v2;
						}
					};
			
			//sort the result
			List<java.util.Map.Entry<String, Integer>> listDoc =
					new ArrayList<java.util.Map.Entry<String, Integer>>(hm.entrySet());
			Collections.sort(listDoc, valueComparator);
			
			//create output string
			StringBuilder builder = new StringBuilder();
			for (java.util.Map.Entry<String, Integer> entry : listDoc) {
				builder.append(entry.getKey());
				builder.append(" ");
				builder.append(entry.getValue());
				builder.append(" ");
			}
			
			//output
			Text docListText = new Text(builder.toString());
			output.collect(key, docListText );
		}
	}

	public static void main(String[] args) throws Exception {
		long time = System.currentTimeMillis();
		// input format:
		// hadoop jar invertedindexes.jar InvertedIndexes input output keyword1 keyword2 ...
		JobConf conf = new JobConf(InvertedIndexing.class); // AAAAA is this program’s file name
		conf.setJobName("InvertedIndexing"); // BBBBB is a job name, whatever you like

		conf.setOutputKeyClass(Text.class);
		conf.setOutputValueClass(Text.class);

		conf.setMapperClass(Map.class);
		//conf.setCombinerClass(Reduce.class);
		conf.setReducerClass(Reduce.class);

		conf.setInputFormat(TextInputFormat.class);
		conf.setOutputFormat(TextOutputFormat.class);

		FileInputFormat.setInputPaths(conf, new Path(args[0])); // input directory name
		FileOutputFormat.setOutputPath(conf, new Path(args[1])); // output directory name

		conf.set( "argc", String.valueOf( args.length - 2 ) ); // argc maintains #keywords
		for ( int i = 0; i < args.length - 2; i++ )
			conf.set( "keyword" + i, args[i + 2] ); // keyword1, keyword2, ...

			JobClient.runJob(conf);
			System.out.println("Elapsed time = " + (System.currentTimeMillis() - time) + " ms");
	}
	
}
