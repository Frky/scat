
class PintoolNotFound(Exception):
    """
        When a given pintool is called but not declared in config file

    """
    pass


class PintoolFileNotFound(Exception):
    """
        When a given pintool is declared in config file but an entry is missing (either src or obj)"

    """
    pass


