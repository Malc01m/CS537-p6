# Workload Generator
You can use `gen_workload.py` to generate workloads and test your key-value store implementation.
This script will generate a text file named `workload.txt` with one request in each line. The line format is as follows:
```
<request type> <key> <value>
```
For example:
```
put 4 5
get 3
put 3 6
get 4
```
Run the script with `-h` to see the possible input options.
It also generates another file called `solution.txt` which has the result of all the get requests in the order that they appear in `workload.txt`. For example, the corresponding `solution.txt` file for the above example would be:
```
0
5
```
If you set the `-c` option when calling the client, it will validate the correctness of the results it got from the server. Note that this check would only be meaningful if you have a single request in flight (`-n 1 -w 1`).
