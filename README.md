# SLC: Static Lisp Compiler

This is a very basic LISP-like language that is statically
typed and compiles into executables.

# Operations

There are several operators supported.

## Unary Operators

There are a few unary operators:

| Operator | Type                 | Description                |
|:---------|:--------------------:|:--------------------------:|
| `not`    | `bool`               | logical not                |
| `car`    | `list<T> -> T`       | returns the head of a list |
| `cdr`    | `list<T> -> list<T>` | returns the tail of a list |

## List operators

| Operator | Type              | Description               |
|:---------|:-----------------:|:-------------------------:|
| `+`      | `list<T> -> T`    | sum of a list             |
| `-`      | `list<T> -> T`    | difference of a list      |
| `*`      | `list<T> -> T`    | product of a list         |
| `/`      | `list<T> -> T`    | division of a list        |
| `and`    | `list<T> -> bool` | logical and'ing of a list |
| `or`     | `list<T> -> bool` | logical or'ing of a list  |
| `xor`    | `list<T> -> bool` | logical xor'ing of a list |

## Binary operators

| Operator | Type                     | Description                                |
|:---------|:------------------------:|:------------------------------------------:|
| `>`      | `T x T -> bool`          | greater than                               |
| `<`      | `T x T -> bool`          | less than                                  |
| `>=`     | `T x T -> bool`          | greater than or equal to                   |
| `<=`     | `T x T -> bool`          | less than or equal to                      |
| `=`      | `T x T -> bool`          | equal to                                   |
| `cons`   | `T x list<T> -> list<T>` | construct a list `(cons 1 '(2)) == '(1 2)` |

# Types

| Type           | C-Type              | Description          |
|:---------------|:-------------------:|:--------------------:|
| `int`          | `int64_t`           | integer              |
| `bool`         | `int8_t`            | boolean              |
| `float`        | `double`            | floating point value |
| `string`       | `const char *`      | text                 |
| `list<int>`    | `slc_int_list *`    | list of integers     |
| `list<float>`  | `slc_double_list *` | list of floats       |
| `list<string>` | `slc_string_list *` | list of strings      |
| `lambda`       |                     | anonymous function   |

# Definitions

| Type           | description          | example                           |
|:---------------|:--------------------:|-----------------------------------|
| `defun`        | define a function    | `(defun main)`                    |
| `extern`       | declare a C function | `(extern int slc_puts(s: string)` |
| `let`          | declare a variable   | `(let x (+ 1 2))`                 |

# Examples

## 1. Hello, World!

I think it's required by law, so here's hello world:

```lisp
(extern int slc_puts(s: string))
(defun main (slc_puts "Hello, world!"))
```

fascinating.

## 2. Basic fibonacci

Using the classical `fn = f_{n - 1} + f_{n - 2}` definition, this is ye old
fibonacci.

```lisp
(defun fib (n: int)  ;; n is the index you want
  (if (< n 2)                          ;; if n is less than 2,
    1                                  ;; then return 1.
    (+ (fib (- n 1)) (fib (- n 2)))))  ;; else, return the sum of fib(n - 1) and fib(n - 2).
```

## 3. Extract two elements from a list

This one is actually where some power becomes visible.

```lisp
(defun extract_two (l: list<int>)
 '((car l) (car (cdr l))))
```

The input of the function is a list of integers, `l`. The return value of this function
is a list `'()` whose elements are verbatim `(car l)` and `(car (cdr l))`.

**note**: this is **not** what `'` means in common lisp, but it's a similar idea.

`(car l)` will return the head of the list `l`, so we have the first element.

`(cdr l)` returns the _rest_ of the list `l` (starting with the second element).
So, if we apply a `(car` to this, we get the second element of the list.

Finally, we construct a new list `'(a b)` with the two values we collected.
