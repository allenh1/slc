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

#include <asw/runtime/slc_double_list.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct slc_double_list
{
  double head;
  struct slc_double_list * tail;
};

struct slc_double_list * slc_double_list_create()
{
  struct slc_double_list * ret = malloc(sizeof(*ret));
  return ret;
}

int8_t slc_double_list_destroy(struct slc_double_list * list)
{
  if (NULL == list) {
    return 0;
  }
  free(list);
  return 1;
}

int8_t slc_double_list_init(struct slc_double_list * list)
{
  list->head = 0.0;
  list->tail = NULL;
  return 1;
}

int8_t slc_double_list_fini(struct slc_double_list * list)
{
  struct slc_double_list ** tail = &list->tail;
  for (; (*tail)->tail != NULL; tail = &(*tail)->tail) {
    if (!slc_double_list_destroy(*tail)) {
      return 0.0;
    }
  }
  return 1;
}

int8_t slc_double_list_set_head(struct slc_double_list * list, double val)
{
  if (NULL == list) {
    return 0;
  }
  list->head = val;
  return 1;
}

int8_t slc_double_list_set_tail(struct slc_double_list * list, struct slc_double_list * tail)
{
  if (NULL == list) {
    return 0;
  }
  list->tail = tail;
  return 1;
}

struct slc_double_list * slc_double_list_cons(double head, struct slc_double_list * tail)
{
  struct slc_double_list * ret = slc_double_list_create();
  if (!slc_double_list_init(ret)) {
    return NULL;
  }
  ret->head = head;
  ret->tail = tail;
  return ret;
}

struct slc_double_list * slc_double_list_append(struct slc_double_list * list, double val)
{
  if (NULL == list) {
    list = slc_double_list_create();
    list->head = val;
    list->tail = NULL;
    return list;
  }
  struct slc_double_list * iter = list;
  for (; iter->tail != NULL; iter = iter->tail) {
    /* go to the end of the list */
  }
  iter->tail = slc_double_list_create();
  slc_double_list_init(iter->tail);
  iter->tail->head = val;
  return list;
}

double * slc_double_list_car(struct slc_double_list * list)
{
  if (list) {
    return &list->head;
  }
  return NULL;
}

struct slc_double_list * slc_double_list_cdr(struct slc_double_list * list)
{
  if (list) {
    return list->tail;
  }
  return NULL;
}

double slc_double_list_add(struct slc_double_list * list)
{
  if (NULL == list) {
    return 0.0;
  }
  struct slc_double_list * tail = list->tail;
  double sum = list->head;
  for (; tail != NULL; tail = tail->tail) {
    sum += tail->head;
  }
  return sum;
}

double slc_double_list_subtract(struct slc_double_list * list)
{
  if (NULL == list) {
    return 0.0;
  }
  struct slc_double_list ** tail = &list->tail;
  double diff = list->head;
  for (; *tail != NULL; tail = &(*tail)->tail) {
    diff -= (*tail)->head;
  }
  return diff;
}

double slc_double_list_multiply(struct slc_double_list * list)
{
  if (NULL == list) {
    return 0.0;
  }
  struct slc_double_list ** tail = &list->tail;
  double prod = list->head;
  for (; *tail != NULL; tail = &(*tail)->tail) {
    prod *= (*tail)->head;
  }
  return prod;
}

double slc_double_list_divide(struct slc_double_list * list)
{
  if (NULL == list) {
    return 0.0;
  }
  struct slc_double_list ** tail = &list->tail;
  double div = list->head;
  for (; *tail != NULL; tail = &(*tail)->tail) {
    div /= (*tail)->head;
  }
  return div;
}

int64_t print_double(double X)
{
  return printf("%lf\n", X);
}
