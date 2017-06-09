#-*- coding: utf-8 -*-

import glob
import os
import subprocess


def checkpath(fpath, **kwargs):
    """
        Perform some verifications of a path (e.g. exists? is a directory?)

        @param fpath            path of the file/dir to check

        @param (opt) isdir      True => check if fpath is directory

        @param (opt) isexec     True => check if fpath is executable

        @raise ValueError if some error occur

    """
    # By default, file is not required to be a directory
    isdir = False
    if "isdir" in kwargs.keys():
        isdir = kwargs["isdir"]

    # By default, file is not required to be executable
    isexec = False
    if "isexec" in kwargs.keys():
        isexec = kwargs["isexec"]

    # Check if path is not empty
    if fpath == "":
        raise ValueError("You must specify a path")

    # If executable, check if we can execute (exists + permission X)
    if isexec and subprocess.call("type " + fpath, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) != 0:
        raise ValueError("Specified target ({0}) is not found or not executable - check permissions".format(fpath))
    else:
        return

    # Check path existance
    if not os.path.exists(fpath):
        raise ValueError("Specified target ({0}) does not exist".format(fpath))

    # If dir is mandatory, perform additional check
    if isdir and not os.path.isdir(fpath):
        raise ValueError("Specified target ({0}) is not a directory".format(fpath))


def complete_bin(text, line, begidx, endidx):
    """
        Autocompletion for executable files

    """
    if line[begidx-1] == '/':
        return complete_path(text, line, begidx, endidx)

    paths = list()
    for path in glob.glob("/usr/bin/" + text + "*"):
        if os.path.isdir(path):
            continue
        paths.append(path.replace("/usr/bin/", ""))
    for path in glob.glob("/bin/" + text + "*"):
        if os.path.isdir(path):
            continue
        paths.append(path.replace("/bin/", ""))
    return paths


def complete_path(text, line, begidx, endidx):
    before_arg = line.rfind(" ", 0, begidx)
    if before_arg == -1:
        return
    fixed = line[before_arg+1:begidx]
    arg = line[before_arg+1:endidx]
    pattern = arg + "*"
    paths = list()
    for path in glob.glob(pattern):
        if os.path.isdir(path) and path[-1] != os.sep:
            path += os.sep
        paths.append(path.replace(fixed, "", 1))
    return paths


def get_pgm_list(logdir, inf_code=None):
    file_list = [f for f in os.listdir(logdir) if f.endswith("log")]
    pgm_list = set(map(lambda a: (a.split("_")[0], a.split("_")[1]), file_list))
    pgm_inf = dict()
    for pgm, inf in pgm_list:
        if pgm not in pgm_inf.keys():
            pgm_inf[pgm] = list()
        pgm_inf[pgm].append(inf)
    return pgm_inf


def get_pgm_and_inf(s, pintools, logdir):
    args = s.split()
    if len(args) == 0 or args[0] == '':
        for p, inf in get_pgm_list(logdir).items():
            print(p)
        raise ValueError
    pgm = args[0]
    if len(args) == 1:
        for p, inf in get_pgm_list().items():
            if p != pgm:
                continue
            for i in inf:
                print(i)
            raise ValueError
    # This line might raise KeyError if pintool is not found
    # This exception should be handled by the caller
    pintool = pintools[args[1]]
    return pgm, pintool


def list_split(l, e):
    res = list()
    curr = list()
    if l.count(e) == 0:
        return l
    for a in l[l.index(e):]:
        if a != e:
            curr.append(a)
        elif len(curr) > 0:
            res.append(curr)
            curr = list()
    if len(curr) > 0:
        res.append(curr)
    return res


def complete_pgm_pintool(text, line, logdir, complete_pintool):
    pgm_inf = get_pgm_list(logdir)
    for p, inf in pgm_inf.items():
        if line.find(p) >= 0:
            if complete_pintool:
                return [i for i in inf if i.startswith(text)]
            return
    return [pgm for pgm, inf in pgm_inf.items() if pgm.startswith(text)]
