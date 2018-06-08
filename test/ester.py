import subprocess, time, sys, os

# ester Context Manager
class Ester(object):
    LEVEL = 'trace'

    def __init__(self, debug=os.getenv('MAROCCO_DEBUG', False)):
        self.debug = debug

    def __enter__(self):
        self.p = subprocess.Popen([
            'ester',
            '--margs',
            '%s' % '--debug' if self.debug else '',
            '--loglevel',
            'sthal:info', # sthal loglevel to info
            'marocco:%s' % Ester.LEVEL,
            'config:%s' % Ester.LEVEL,
            ])
        # wait two seconds for ester and rcf to come up
        time.sleep(2)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        print self.p.poll()
        if self.p.poll():
            raise RuntimeError("ester server didn't survive")
        self.kill()

    def running(self):
        return self.p.poll() == None

    def exit_code(self):
        return self.p.poll()

    def kill(self):
        self.p.kill()
        return self.exit_code()
