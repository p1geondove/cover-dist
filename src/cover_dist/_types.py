from dataclasses import dataclass
from enum import Enum

class Status(Enum):
    OK = 0
    ERR_NO_RADIX = 1
    ERR_MULTIPLE_RADIX = 2
    ERR_INVALID_CHAR = 4
    ERR_ALLOCATION = 8
    ERR_OPEN_FAILED = 16
    ERR_INSUFFICIENT_DIGITS = 32
    ERR_TOO_MANY_DIGITS = 64
    ERR_INTERRUPT = 128

@dataclass
class FileMeta:
    size:int
    radix_pos:int
    zero_offset:int
    status:Status
