# TimeKeeper (tike)

## What is it?
    This is a simple task keeping application written in c++
    I decided to write it to learn c++

## Help page
    Usage: Tike [OPTIONS]

    TimeKeeper
    
    Options:
        -a, --add                 Add a new task
        -c, --complete            Mark a task as completed by id
        -d, --description         Description of the task
        -h, --help                Show this help page
        -l, --list                List a task by id
        -L, --list-all            List all tasks
            --list-all-completed  List all completed tasks
            --list-completed      List a completed task by id
        -r, --remove              Remove a task by id
        -t, --title               Title of the task
        -v, --version             Prints the version number

## Dependencies
    This is only depenent on Sqlite3
```bash
# REHL/Fedora
sudo dnf install sqlite3-devel -y

# Debian/Ubuntu
sudo apt isntall sqlite3-devel -y
````

## How to compile
```bash
mkdir build
cd build
cmake ..
cmake --build .

# After that you can move tike to /usr/bin/tike if you want to install it globally
```