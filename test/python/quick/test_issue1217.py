#!/usr/bin/env python
import unittest

class Issue1217(unittest.TestCase):

    def test(self):

        import pyhmf as sim
        from pymarocco import PyMarocco
        marocco = PyMarocco()
        sim.setup(marocco=marocco)

if __name__ == "__main__":
    unittest.main()
