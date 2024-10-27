// Copyright 2024 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <asw/runtime/slc_int_list.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct slc_int_list
{
  int64_t head;
  struct slc_int_list * tail;
};

struct slc_int_list * slc_int_list_create()
{
  struct slc_int_list * ret = malloc(sizeof(*ret));
  return ret;
}

int8_t slc_int_list_destroy(struct slc_int_list * list)
{
  if (NULL == list) {
    return 0;
  }
  free(list);
  return 1;
}

int8_t slc_int_list_init(struct slc_int_list * list)
{
  list->head = 0;
  list->tail = NULL;
  return 1;
}

int8_t slc_int_list_fini(struct slc_int_list * list)
{
  struct slc_int_list ** tail = &list->tail;
  for (; (*tail)->tail != NULL; tail = &(*tail)->tail) {
    if (!slc_int_list_destroy(*tail)) {
      return 0;
    }
  }
  return 1;
}

int8_t slc_int_list_set_head(struct slc_int_list * list, int64_t val)
{
  if (NULL == list) {
    return 0;
  }
  list->head = val;
  return 1;
}

int8_t slc_int_list_set_tail(struct slc_int_list * list, struct slc_int_list * tail)
{
  if (NULL == list) {
    return 0;
  }
  list->tail = tail;
  return 1;
}

struct slc_int_list * slc_int_list_cons(int64_t head, struct slc_int_list * tail)
{
  struct slc_int_list * ret = slc_int_list_create();
  if (!slc_int_list_init(ret)) {
    return NULL;
  }
  ret->head = head;
  ret->tail = tail;
  return ret;
}

int64_t * slc_int_list_car(struct slc_int_list * list)
{
  if (list) {
    return &list->head;
  }
  return NULL;
}

struct slc_int_list * slc_int_list_cdr(struct slc_int_list * list)
{
  if (list) {
    return list->tail;
  }
  return NULL;
}

int64_t slc_int_list_add(struct slc_int_list * list)
{
  if (NULL == list) {
    return 0;
  }
  struct slc_int_list ** tail = &list->tail;
  int64_t sum = list->head;
  for (; *tail != NULL; tail = &(*tail)->tail) {
    sum += (*tail)->head;
  }
  return sum;
}

int64_t slc_int_list_subtract(struct slc_int_list * list)
{
  if (NULL == list) {
    return 0;
  }
  struct slc_int_list ** tail = &list->tail;
  int64_t diff = list->head;
  for (; *tail != NULL; tail = &(*tail)->tail) {
    diff -= (*tail)->head;
  }
  return diff;
}

int64_t slc_int_list_multiply(struct slc_int_list * list)
{
  if (NULL == list) {
    return 0;
  }
  struct slc_int_list ** tail = &list->tail;
  int64_t prod = list->head;
  for (; *tail != NULL; tail = &(*tail)->tail) {
    prod *= (*tail)->head;
  }
  return prod;
}

int64_t slc_int_list_divide(struct slc_int_list * list)
{
  if (NULL == list) {
    /* this is probably correct most of the time anyway */
    return 0;
  }
  struct slc_int_list ** tail = &list->tail;
  int64_t div = list->head;
  for (; *tail != NULL; tail = &(*tail)->tail) {
    div /= (*tail)->head;
  }
  return div;
}

int32_t print_int(int64_t i)
{
  return printf("%Ld\n", i);
}