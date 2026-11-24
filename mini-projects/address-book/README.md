# Address Book in C (CSV File)

An address book kept in a CSV file that Excel, LibreOffice or any text editor can open. Contacts are loaded from the file at start-up; you can list them sorted by name or by email, add, search and delete them, and save when you choose Save or when you quit and answer yes. Each contact has a name, a phone number and an email address, and names may contain commas and quotes, such as `Smith, John` or `Ana "Ani" Lopez`: the CSV file stores them correctly. Saving is safe, too: the program writes a temporary file first and only replaces the old one when the new one is complete.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#address-book) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o address_book address_book.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 address_book.c
```

## Run

```sh
./address_book                   # uses contacts.csv
./address_book --file work.csv   # a second address book
```

On Windows, run `address_book.exe` the same way.

## Sample session

With two contacts already saved, listing them by email:

```text
Address Book
============
Loaded 2 contacts from contacts.csv.

1. List contacts by name
2. List contacts by email
3. Add a contact
4. Search
5. Delete a contact
6. Save
0. Quit
Choose an option (0-6): 2

  #  Name                      Phone                 Email
  2  Smith, John               +44 20 7946 0000      js@example.co.uk
  1  Ana "Ani" Lopez           (555) 010-0199        lopez.ana@example.com

1. List contacts by name
2. List contacts by email
3. Add a contact
4. Search
5. Delete a contact
6. Save
0. Quit
Choose an option (0-6): 0

Goodbye!
```

The numbers are always each contact's place in name order, so they stay the same in every list and are the numbers Delete asks for.

## The CSV format

The first line is `name,phone,email`, then one contact per line. A field that contains a comma or a quote is written inside quotes, with every `"` doubled: `"Ana ""Ani"" Lopez",(555) 010-0199,lopez.ana@example.com`.

## What you practice

- An array of structs kept in order, inside an `AddressBook` struct with a count
- `qsort` with comparison functions such as `compare_by_name()`: passing a function to a function, and casting `const void *` back to `const Contact *`
- Comparing strings while ignoring case: `strcmp` puts `Smith` before `ben`, `compare_ignore_case()` does not
- Copying structs with `=`: the email list sorts a copy, so the stored order never changes
- Deleting from an array by shifting the later elements down with `memmove`
- CSV done properly: quoting fields that contain commas or quotes, and reading them back
- A safe save: write `FILE.tmp`, check it, then `remove()` the old file and `rename()` the new one
- Validating a phone number and an email address (simply: real email validation is much harder)
- Remembering unsaved changes with a `bool` and asking before quitting

## Tests

The files in `tests/` are typed input; the matching `.expected.txt` files are the exact output. Each test runs in its own build folder and deletes its CSV file first; `persist1` saves two contacts and `persist2` loads them back. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Edit a contact.
- More fields, such as an address or a birthday.
- Grow the array with `realloc` instead of a fixed limit of 100.
- Import contacts from another CSV file and merge them.
