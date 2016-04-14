from pygccxml.declarations import class_t, reference_t
from pyplusplus.module_builder import call_policies


def expose_iterator_interface(cl):
    """
    Expose iterator interface instead of raw begin()/end().
    """
    begin_funs = list(cl.mem_funs("begin", allow_empty=True))
    end_funs = list(cl.mem_funs("end", allow_empty=True))
    if not (begin_funs and end_funs):
        return False
    for fun in begin_funs + end_funs:
        fun.exclude()
    cl.add_registration_code(
        'def("__iter__", bp::iterator< {} >())'.format(cl.decl_string))
    return True


def builder_pattern(ns, name="Builder"):
    for cl in ns.classes(name, allow_empty=True):
        # chainable calls
        for fun in cl.mem_funs(allow_empty=True):
            if not isinstance(fun.return_type, reference_t):
                continue
            if not fun.return_type.base.declaration == cl:
                continue
            fun.call_policies = call_policies.return_self()
        # Explicitly set noncopyable of parent (i.e. built type) to
        # False.  This is necessary, because parent may not have
        # public constructors, which leads py++ to the (wrong)
        # conclusion that the class is noncopyable.
        if isinstance(cl.parent, class_t):
            cl.parent.noncopyable = False
    else:
        return False
    return True
