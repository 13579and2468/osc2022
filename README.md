# Operating Systems Capstone 2022

## Author

| Student ID | GitHub account name | My name |
| --- | ----------- | --- |
|`310551097`| `13579and2468` | `曾思維` |

## spec

https://oscapstone.github.io/

## uncomplete parts

lab6 advanced 3 : copy on write
lab8 : all

## compile
```
make
```
## compile with debug symbol
```
make CFLAGS='-g' ASMFLAGS='-g'
```
## run
```
make run
```
## debug with gdb

```
make run_debug
```
(in another terminal)
```
gdb
file build/kernel8.elf 
target remote :1234
```
