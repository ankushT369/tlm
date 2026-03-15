
# tlm
tlm is a terminal link manager where you can store, manage, list, and search your links efficiently.  
It uses a local SQLite database and a simple command-based interface with history support.

## Features
- Store links with titles
- Create custom tables
- Show all tables or a specific table
- Add, remove, and update entries
- Search through titles using a lightweight ncurses-based fuzzy search
- Persistent history saved automatically to the database using linenoise

## Build
Make sure `sqlite3.c` and `sqlite3.h` are in the project root.

Build in release mode:
```

make

```

Build in debug mode:
```

make debug

```

Clean:
```

make clean

```

## Running
Run the program:
```

./tlm

```

A prompt will appear:
```

tlm>

```

Enter commands as described below.

## Commands

### 1. show
List database tables or display all rows from a table.

Show all tables:
```

show tlm

```

Show contents of a table:
```

show table_name

```

### 2. create
Create a new table with an auto-increment id, link, and title.

Example:
```

create mylinks

```

Reserved table names:
- tlm
- tlm_history

### 3. add
Insert a new link into a table.

Syntax:
```

add table_name link "title with spaces"

```

Example:
```

add mylinks <https://example.com> "Example Website"

```

Space handling inside quotes is supported.

### 4. rm
Remove rows based on a condition.

Syntax:
```

rm table_name condition

```

Example:
```

rm mylinks id=3

```
### 5. update
Update columns in a table.

Basic example:
```

update mylinks link=www.google.com title=google

```

Note: current update implementation does not enforce WHERE usage, so use carefully.

### 6. Built-in slash commands
These start with `/`.

Show or change history length:
```

/historylen 200

```

Enable password masking:
```

/mask

```

Disable masking:
```

/unmask

```

Launch fuzzy search UI:
```

/search

```

Exit with Ctrl+D or Ctrl+C.

## Search Mode
The `/search` command opens a minimal ncurses-based interface that lets you:

- Type to filter items (subsequence match)
- Navigate with up/down arrows
- Press Enter to select an item
- Press ESC to exit

Search entries are loaded from `title.txt`.

## Database
The SQLite database file is `tlm.db`.

The program automatically creates:
```

tlm_history (id, command)

```

This stores command history and loads it during startup.

## Notes and Limitations
- SQL injection protection is limited; avoid untrusted input.
- UPDATE queries do not currently require a WHERE clause.
- The table renderer calculates widths only from headers, so long values may wrap visually.
