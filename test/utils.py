import inspect
import functools


def parametrize(params):
    def decorator(method):
        for param in params:
            def wrapper(self, parameter=param):
                method(self, parameter)

            functools.update_wrapper(wrapper, method)

            new_method_name = '{}_{}'.format(method.__name__, param)
            wrapper.__name__ = new_method_name

            frame = inspect.currentframe(1)
            frame.f_locals[new_method_name] = wrapper
        return None
    return decorator
