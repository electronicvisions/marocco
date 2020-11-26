import subprocess

import unittest

class TestTools(unittest.TestCase):
    def test_find_route(self):
        """
        Integration test for find_route tool.
        """
        base_shell_command = ["find_route.py", "--source-hicann", "0", "--target-hicann", "10"]

        shell_command = base_shell_command + ["--source-vline", "10"]
        subprocess.check_call(' '.join(shell_command), shell=True)

        shell_command = base_shell_command + ["--source-hline", "10", "--target-vertical"]
        subprocess.check_call(' '.join(shell_command), shell=True)

if __name__ == '__main__':
    unittest.main()
