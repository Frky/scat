
import os
import subprocess
import shutil
from datetime import datetime

class Pintool(object):

    nb_pintools = 0

    def __init__(self, *args, **kwargs):
        # Check for required parameters in kwargs
        req_param = ["name", "src_path", "obj_path", "pinconf", "log_dir"]
        for param in req_param:
            if param not in kwargs.keys():
                print "ERROR: parameter {0} expected".format(param)
                raise Exception
        self.__name = kwargs["name"]
        self.__src_path = kwargs["src_path"]
        self.__obj_path = kwargs["obj_path"]
        self.__pinconf = kwargs["pinconf"]
        self.__logdir = kwargs["log_dir"]

        # LOG function provided?
        if "stdout" in kwargs.keys():
            self.stdout = kwargs["stdout"]
        else:
            # If not, use the local one
            self.stdout = self.__log
        if "stderr" in kwargs.keys():
            self.stderr = kwargs["stderr"]
        else:
            # If not, use the local one
            self.stderr = self.__log

        # Previous step required?
        if "prev_step" in kwargs.keys():
            self.__prev_step = kwargs["prev_step"]
        else:
            self.__prev_step = None

        # Set pintool code
        self.__code = Pintool.nb_pintools
        # Increment number of pintools created
        Pintool.nb_pintools += 1

    @property
    def code(self):
        return self.__code

    def __str__(self):
        return self.__name

    def __log(self, msg):
        print msg

    @property
    def prev_step(self):
        """
            Which pintool does it require to be executed before?
            return the name of the inference that preceeds

        """
        return self.__prev_step

    def __cmd(self, binary, args, logfile, infile=None):
        if infile is not None:
            infile_opt = "-i {0}".format(infile)
        else:
            infile_opt = ""
        if "cli-options" in self.__pinconf.keys():
            cli_options = self.__pinconf["cli-options"]
        else:
            cli_options = ""
        return "{0} {4} -t {1} -o {2} {3} -- {5} {6}".format(
                                                                            self.__pinconf["bin"],
                                                                            self.__obj_path,
                                                                            logfile,
                                                                            infile_opt,
                                                                            cli_options,
                                                                            binary,
                                                                            " ".join(args),
                                                                        )

    def __gen_logfile(self, binary):
        """
            Generate a name for a new log file. For example, for arity inference
            on "grep", this will return grep_arity_{timestamp}.log.

            @param binary       name of the binary for which we require a log

            @ret                the generated name for the log file.

        """
        timestamp = datetime.now().strftime("%s")
        return "{3}/{0}_{1}_{2}.log".format(os.path.basename(binary), str(self), timestamp, self.__logdir)

    def get_logfile(self, binary, prev=True):
        """
            Retrieve the most recent logfile from the given step of inference.

            @param binary   the binary file to analyse

            @param prev     If true, retrieve the log file of the previouis step
                            Otherwise, retrieve the log for the current step

            @ret            a path to the most recent logfile from step

            @raise IOError  if no file from step is found.

        """
        inf = self.__prev_step if prev else self
        candidates = map(
                                lambda x:
                                    "{0}/{1}".format(self.__logdir, x),
                                os.listdir(self.__logdir),
                        )
        candidates = filter(
                                lambda x:
                                    (x.startswith("{2}/{0}_{1}".format(
                                                                            os.path.basename(binary),
                                                                            inf,
                                                                            self.__logdir)
                                                                        ) and
                                    x.endswith(".log")),
                                candidates,
                            )
        if len(candidates) == 0:
            self.stderr("Cannot file result from {0} inference - ensure that you did run every step in order (arity > type > couple) for this binary".format(self))
            raise IOError
        return max(candidates, key=os.path.getmtime)

    def launch(self, binary, args, verbose=True):
        """
            Launch specified inference on binary given in parameter

            @param binary   the binary file to analyse (must be a valid path to
                            an executable)

            @param args     arguments to give to the binary

            @param verbose  if True, print intermediate steps

        """
        # if inf_code == INF_BASE:
        #     cmd = "{0} {1}".format(binary, " ".join(args))
        # else:
        logfile = self.__gen_logfile(binary)
        if self.__prev_step is not None:
            infile = self.get_logfile(binary)
        else:
            infile = None
        cmd = self.__cmd(binary, args, logfile, infile)
        self.stdout(cmd, verbose)
        start = datetime.now()
        subprocess.call(cmd, shell=True)
        duration = datetime.now() - start
        self.stdout("Inference results logged in {0}".format(logfile), verbose)
        self.stdout("Execution time: {0}.{1}s".format(duration.seconds, duration.microseconds), verbose)


    def compile(self, force, debug, trace):
        """
            Compile all pintools needed

        """
        #TODO os independant

        force_flag = ''
        if force:
            force_flag = '-B'

        src_path, src_name = os.path.split(self.__src_path)
        obj_path, obj_name = os.path.split(self.__obj_path)
        obj_path = os.path.abspath(obj_path)

        obj_build_path = "{}/build".format(obj_path)
        obj_build_name = src_name[:-4]
        if debug or trace:
            if debug:
                obj_build_name += '-debug'
            if trace:
                obj_build_name += '-trace'
        else:
            obj_build_name += '-release'

        if not os.path.exists(obj_build_path):
            os.makedirs(obj_build_path)

        obj_build_file = "{}/{}.so".format(obj_build_path, obj_build_name)
        if os.path.exists(obj_build_file):
            mtime_before = os.stat(obj_build_file).st_mtime
        else:
            mtime_before = 0

        if "compile-flags" in self.__pinconf.keys():
            compile_flags = self.__pinconf["compile-flags"]
        else:
            compile_flags = ""
        cmd = "make {} PIN_ROOT='{}' SCAT_COMPILE_FLAGS='{}' OBJDIR='{}/' '{}/{}.so'".format(
                force_flag,
                self.__pinconf["path"],
                compile_flags,
                obj_build_path,
                obj_build_path,
                obj_build_name)

        self.stdout("Compiling pintool: {0} ...".format(src_name[:-4]))
        with open("/dev/null", 'w') as fnull:
            try:
                subprocess.check_call(cmd, cwd=src_path, shell=True, stdout=fnull)
                mtime_now = os.stat("{}/{}.so".format(obj_build_path, obj_build_name)).st_mtime
                shutil.copyfile(
                        '{}/{}.so'.format(obj_build_path, obj_build_name),
                        '{}/{}'.format(obj_path, obj_name))
                if mtime_before == mtime_now:
                    self.stdout("\t=> Up to date !")
                else:
                    self.stdout("\t=> Done !")
            except subprocess.CalledProcessError as error:
                self.stdout("/!\ Compilation exited with non-zero status {} /!\\\n\n".format(error.returncode))
