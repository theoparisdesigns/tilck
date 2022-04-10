# SPDX-License-Identifier: BSD-2-Clause

import gdb # pylint: disable=import-error
from . import base_utils as bu
from . import tilck_types as tt

WOBJ_NONE = gdb.parse_and_eval("WOBJ_NONE")
WOBJ_TASK = gdb.parse_and_eval("WOBJ_TASK")
WOBJ_MWO_WAITER = gdb.parse_and_eval("WOBJ_MWO_WAITER")

class printer_mwobj_elem:

   def __init__(self, val):
      self.val = val

   def to_string(self):
      return "mwobj_elem"

   def children(self):
      e = self.val
      wobj = e["wobj"]

      return [
         ("type", e["type"]),
         ("extra", wobj["extra"]),
         ("ptr", wobj["__ptr"]),
      ]

class printer_multi_obj_waiter:

   def __init__(self, val):
      self.val = val

   def display_hint(self):
      return 'array'

   def children(self):

      mwobj = self.val
      count = mwobj['count']
      elems = mwobj['elems']
      arr = []

      for i in range(count):
         arr.append((str(i), elems[i]))

      return arr

   def to_string(self):
      return bu.fmt_type("struct multi_obj_waiter", self.val)

class printer_wait_obj:

   def __init__(self, val):
      self.val = val

   def children(self):

      wobj = self.val

      if wobj['type'] == WOBJ_NONE:
         return []

      res = [
         ("type", wobj['type']),
         ("extra", wobj['extra']),
      ]

      if wobj['type'] == WOBJ_TASK:

         tidval = wobj['__data']

         if tidval < -1:
            tidval = "<any child with pgid = {}>".format(-tidval)
         elif tidval == -1:
            tidval = "<any child>"
         elif tidval == 0:
            tidval = "<any child with same pgid>"

         res.append(("tid", tidval))

      elif wobj['type'] == WOBJ_MWO_WAITER:

         mwobj = wobj['__ptr'].cast(tt.multi_obj_waiter_p)
         res.append(("ptr", mwobj.dereference()))

      else:

         res.append(("ptr", wobj['__ptr']))

      return res


   def to_string(self):
      return bu.fmt_type("struct wait_obj", self.val)

bu.register_tilck_regex_pp(
   'wait_obj', '^wait_obj$', printer_wait_obj
)

bu.register_tilck_regex_pp(
   'multi_obj_waiter', '^multi_obj_waiter$', printer_multi_obj_waiter
)

bu.register_tilck_regex_pp(
   'mwobj_elem', '^mwobj_elem$', printer_mwobj_elem
)
