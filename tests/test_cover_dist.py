import pytest
from pathlib import Path
from coverdist import cover_dist

DIR_TESTNUMS = Path("test_numbers/")
FILE_BIG_INTPART =       DIR_TESTNUMS / "big_intpart.txt"
FILE_EMPTY =             DIR_TESTNUMS / "empty.txt"
FILE_LARGE_ZERO_OFFSET = DIR_TESTNUMS / "large_zero_offset.txt"
FILE_ILLEGAL_CHAR =      DIR_TESTNUMS / "illegal_char.txt"
FILE_MISSING_RADIX =     DIR_TESTNUMS / "missing_radix.txt"
FILE_NOT_ENOUGH_DIGITS = DIR_TESTNUMS / "not_enough_digits.txt"
FILE_PI =                DIR_TESTNUMS / "pi.txt"
FILE_RADIX_AT_0 =        DIR_TESTNUMS / "radix_at_0.txt"
FILE_NONEXISTANT =       DIR_TESTNUMS / "some_nonexistant_file.txt"

def test_pi():
    assert cover_dist(FILE_PI, 1) == (33,0)
    assert cover_dist(FILE_PI, 2) == (607,68)
    assert cover_dist(FILE_PI, 3) == (8556,483)

def test_big_int():
    assert cover_dist(FILE_BIG_INTPART, 1) == (39,9)

# All of below should raise

def test_empty():
    with pytest.raises(ValueError):
        cover_dist(FILE_EMPTY, 1)

def test_illegal_char():
    with pytest.raises(ValueError, match="invalid char found"):
        cover_dist(FILE_ILLEGAL_CHAR, 1)

def test_missing_radix():
    with pytest.raises(ValueError, match="can't find radix point"):
        cover_dist(FILE_MISSING_RADIX, 1)

def test_not_enough_digits():
    with pytest.raises(ValueError, match="not enough digits in file"):
        cover_dist(FILE_NOT_ENOUGH_DIGITS, 1)

def test_nonexistant():
    with pytest.raises(FileNotFoundError):
        cover_dist(FILE_NONEXISTANT, 1)

def test_directory():
    with pytest.raises(IsADirectoryError):
        cover_dist(DIR_TESTNUMS, 1)

def test_permission_denied(tmp_path):
    file = tmp_path / "wrong_permissions.txt"
    file.write_text("3.14159")
    file.chmod(0)

    with pytest.raises(PermissionError):
        cover_dist(file, 1)
