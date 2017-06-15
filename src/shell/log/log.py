from time import time
import os


class LogManager(object):
    """
    """


    def __init__(self, config):
        self._config = config      
        self._logdir = config["log"]["path"]

    def match_logfile(self, dependency, binary, candidate):
        name = "{2}/{0}/{1}/".format(os.path.basename(binary), 
                dependency, self._logdir)
        return candidate.startswith(name) and candidate.endswith(".log")


    def get_log(self, dependency, binary):
        """
            Retrieve the most recent logfile from the given step of inference.

            @param binary   the binary file to analyse

            @ret            a path to the most recent logfile from step

            @raise IOError  if no file from step is found.

        """
        directory = "{}/{}/{}".format(
                self._logdir,
                os.path.basename(binary),
                dependency)

        candidates = list()
        if os.path.exists(directory):
            candidates = map(
                    lambda x: "{}/{}".format(directory, x),
                    os.listdir(directory),
                    )
        if len(candidates) == 0:
            print("*** Cannot find log file for "
                    "{} - ensure that you did run every step in order "
                    "(arity > type > couple > memalloc) for this binary (\"{}\")."
                    .format(dependency, binary))
            raise IOError
        return max(candidates, key=os.path.getmtime)


    def gen_log(self, pin_name, binary, ext="log"):
        """
            Generate a name for a new output file.
        """
        directory = "{}/{}/{}".format(
                self._logdir,
                os.path.basename(binary),
                pin_name)

        if not os.path.exists(directory):
            os.makedirs(directory)

        if "keep" in self._config["log"].keys():
            self.clean_logs(self._config["log"]["keep"])
        return "{}/{}.{}".format(directory, int(time()), ext)

    def clean_logs(self, keep):
        """
            Keep only depth (default to 5) logs by program and by inference
        """
        binaries = os.listdir(self._logdir)
        for binary in os.listdir(self._logdir):
            binary_path = "{}/{}".format(self._logdir, binary)
            for pin_name in os.listdir(binary_path):
                pin_path = "{}/{}".format(binary_path, pin_name)
                log_files = map(lambda x:"{}/{}".format(pin_path, x),
                        os.listdir(pin_path))
                if keep > 0:
                    log_files.sort(key=os.path.getmtime)
                for file_ in log_files[:-keep]:
                    os.remove(file_)
