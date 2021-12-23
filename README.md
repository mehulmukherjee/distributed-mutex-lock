## Project Notes

# General Setup
1) Run `ssh <user>@e5-cse-135-01.cse.psu.edu`\n`ssh <user>@e5-cse-135-16.cse.psu.edu`\n`ssh <user>@e5-cse-135-17.cse.psu.edu`\n`ssh <user>@e5-cse-135-35.cse.psu.edu` (...or modify node\_list.txt with desired nodes)

2) When using `ssh <user>@e5-cse-135-xx.cse.psu.edu`, upon successfully establishing a connection and login, run `bash` immediately.

3) Use the following command to update the PATH to include gRPC-required compilation items:
`source updatePath.sh`

4) run `make -j` to generate all programs (currently dml and dsm...)

- Note: `make clean` will remove all compilation-related files (i.e. auto-generated rpc files and .obj files). `make fresh` will remove compilation-related .obj files, leaving auto-generated rpc files alone.

# DML test execution
1)On each client node (e.g., ...01.cse.psu.edu, ...16.cse.psu.edu, ...17.cse.psu.edu), run the following test command:
`./dml_p1 <lockno>` 
where 'lockno' is an integer between 0 and 7 representing which lock the program will be vying for.

2) Wait 1 second for each client to start outputting to console. The rpc-log-file.txt will be storing the rpc call information.

- Note: `make -j dml` will make files related to the dml part of the project.
