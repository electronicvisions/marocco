import os
import shutil
import tempfile
import inspect
import unittest
import functools

from pymarocco.results import Marocco
from pysthal.command_line_util import init_logger
import pylogging
import pymarocco


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


class TestWithResults(unittest.TestCase):
    def setUp(self):
        init_logger("INFO", [
            ("marocco", "DEBUG"),
        ])

        self.log = pylogging.get(__name__)
        self.temporary_directory = tempfile.mkdtemp(prefix="marocco-test-")

        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = pymarocco.PyMarocco.None
        self.marocco.persist = os.path.join(
            self.temporary_directory, "results.bin")

    def tearDown(self):
        shutil.rmtree(self.temporary_directory, ignore_errors=True)
        del self.marocco

    def load_results(self):
        self.assertTrue(os.path.exists(self.marocco.persist))
        return Marocco.from_file(self.marocco.persist)
