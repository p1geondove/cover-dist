from pathlib import Path
from threading import Lock, Thread
import json

from coverdist import cover_dist

NUM_FILES = list(Path("./nums/").iterdir())
MAX_DIGITS = 7

class SharedMem:
    lock:Lock = Lock()
    res_dict:dict[str,dict[str,list[int]]] = {
        n.name:{
            "distances":[],
            "last_nums":[]
        } for n in NUM_FILES
    }

def _worker(number_file:Path, mem:SharedMem):
    num_name = number_file.name
    last_nums:list[int] = []
    distances:list[int] = []

    for n in range(1,MAX_DIGITS+1):
        dist, last_num = cover_dist(str(number_file), n)
        with mem.lock:
            mem.res_dict[num_name]["distances"].append(dist)
            mem.res_dict[num_name]["last_nums"].append(last_num)

def main():
    mem = SharedMem()

    workers:list[Thread] = []
    for num_file in NUM_FILES:
        t = Thread(target=_worker, args=(num_file, mem))
        workers.append(t)
        t.start()

    for t in workers:
        t.join()

    with Path("results2.json").open("wt") as f:
        json.dump(mem.res_dict, f, indent=4)

if __name__ == "__main__":
    main()
