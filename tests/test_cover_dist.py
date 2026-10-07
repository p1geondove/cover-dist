import pytest
from pathlib import Path
from cover_dist import cover_dist

DIR_TESTNUMS = Path("test_numbers/")
FILE_BIG_INTPART =       DIR_TESTNUMS / "big_intpart.txt"
FILE_EMPTY =             DIR_TESTNUMS / "empty.txt"
FILE_LARGE_ZERO_OFFSET = DIR_TESTNUMS / "large_zero_offset.txt"
FILE_ILLEGAL_CHAR =      DIR_TESTNUMS / "illegal_char.txt"
FILE_MISSING_RADIX =     DIR_TESTNUMS / "missing_radix.txt"
FILE_NOT_ENOUGH_DIGITS = DIR_TESTNUMS / "not_enough_digits.txt"
FILE_E =                 DIR_TESTNUMS / "e.txt"
FILE_RADIX_AT_0 =        DIR_TESTNUMS / "radix_at_0.txt"
FILE_NONEXISTANT =       DIR_TESTNUMS / "some_nonexistant_file.txt"
FILE_STRING =            "test_numbers/e.txt"

def test_big_int():
    assert cover_dist(FILE_BIG_INTPART, 1) == (39,9)

def test_e():
    # im going pretty far to make sure the program reads multiple buffers
    # also the last buffer is only partially filled which should be a nice edge case
    assert cover_dist(FILE_E, 1) == (21, 6)
    assert cover_dist(FILE_E, 2) == (372, 12)
    assert cover_dist(FILE_E, 3) == (8092, 548)
    assert cover_dist(FILE_E, 4) == (102128, 1769)
    assert cover_dist(FILE_E, 5) == (1061613, 92994)
    assert cover_dist(FILE_E, 6) == (12108841, 513311)

def test_string():
    assert cover_dist(FILE_STRING, 1) == (21, 6)


# All of below should raise

def test_allocation_error():
    # if given enough memory this could run, but any sane system (the year is 2026) should run out of memory
    # this would need over 1 exabyte of memory (10**19/8 bytes)
    with pytest.raises(MemoryError):
        cover_dist(FILE_E, 19)

def test_directory():
    with pytest.raises(IsADirectoryError):
        cover_dist(DIR_TESTNUMS, 1)

def test_empty():
    with pytest.raises(ValueError):
        cover_dist(FILE_EMPTY, 1)

def test_illegal_char():
    with pytest.raises(ValueError, match="invalid char found"):
        cover_dist(FILE_ILLEGAL_CHAR, 1)

def test_invalid_number_of_digits():
    with pytest.raises(ValueError, match="number of digits has to be a positive nonzero integer"):
        cover_dist(FILE_E, 0)
    with pytest.raises(ValueError, match="number of digits has to be a positive nonzero integer"):
        cover_dist(FILE_E, -1)

def test_missing_radix():
    with pytest.raises(ValueError, match="can't find radix point"):
        cover_dist(FILE_MISSING_RADIX, 1)

def test_not_enough_digits():
    with pytest.raises(ValueError, match="not enough digits in file"):
        cover_dist(FILE_NOT_ENOUGH_DIGITS, 1)

def test_nonexistant():
    with pytest.raises(FileNotFoundError):
        cover_dist(FILE_NONEXISTANT, 1)

def test_overflow():
    with pytest.raises(OverflowError):
        cover_dist(FILE_E, 10000000000000000000)

def test_permission_denied(tmp_path):
    file = tmp_path / "wrong_permissions.txt"
    file.write_text("3.14159")
    file.chmod(0)

    with pytest.raises(PermissionError):
        cover_dist(file, 1)

def test_too_many_digits():
    with pytest.raises(ValueError, match=r"number of digits has to be an integer between 1 and 19 \(both included\)"):
        cover_dist(FILE_E, 20)

def test_type_error():
    with pytest.raises(TypeError, match="expected str, bytes or os.PathLike object, not int"):
        cover_dist(123,1) #type:ignore

