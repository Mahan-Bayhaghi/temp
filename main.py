import subprocess

# start train.py and train.c
train_process = subprocess.Popen(["python3","train.py"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
train_c_process = subprocess.Popen(["./train"], stdin=train_process.stdout)

# start aggregate.py and aggregate.c
aggregate_process = subprocess.Popen(["python3","aggregate.py"], stdin=train_c_process.stdout, stdout=subprocess.PIPE)
aggregate_c_process = subprocess.Popen(["./aggregate"], stdin=aggregate_process.stdout)

# wait for system to finish
aggregate_c_process.wait()