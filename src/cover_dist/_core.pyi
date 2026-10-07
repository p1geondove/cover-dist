from pathlib import Path
from ._types import FileMeta, Status


def cover_dist(file_path:Path|str|bytes, num_digits:int) -> tuple[int,int]:
    """ Returns the number of digits needed as well as the last seen number to cover all n-digit numbers """


def cancel() -> None:
    """ Stop all current and future scans """


def get_metadata(file_path:Path|str|bytes) -> FileMeta:
    """ Returns metadata like filesize, position of radix point, zero_offset and Status """
