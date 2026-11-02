"""Terminal UI utilities for build scripts."""

import sys
import os


class Colors:
    """ANSI color codes."""
    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    
    # Foreground colors
    BLACK = "\033[30m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    MAGENTA = "\033[35m"
    CYAN = "\033[36m"
    WHITE = "\033[37m"
    
    # Bright foreground colors
    BRIGHT_BLACK = "\033[90m"
    BRIGHT_RED = "\033[91m"
    BRIGHT_GREEN = "\033[92m"
    BRIGHT_YELLOW = "\033[93m"
    BRIGHT_BLUE = "\033[94m"
    BRIGHT_MAGENTA = "\033[95m"
    BRIGHT_CYAN = "\033[96m"
    BRIGHT_WHITE = "\033[97m"
    
    @staticmethod
    def disable():
        """Disable all colors."""
        Colors.RESET = ""
        Colors.BOLD = ""
        Colors.DIM = ""
        Colors.BLACK = ""
        Colors.RED = ""
        Colors.GREEN = ""
        Colors.YELLOW = ""
        Colors.BLUE = ""
        Colors.MAGENTA = ""
        Colors.CYAN = ""
        Colors.WHITE = ""
        Colors.BRIGHT_BLACK = ""
        Colors.BRIGHT_RED = ""
        Colors.BRIGHT_GREEN = ""
        Colors.BRIGHT_YELLOW = ""
        Colors.BRIGHT_BLUE = ""
        Colors.BRIGHT_MAGENTA = ""
        Colors.BRIGHT_CYAN = ""
        Colors.BRIGHT_WHITE = ""


class TUI:
    """Terminal User Interface for build scripts."""
    
    def __init__(self, ci_mode=False):
        """Initialize TUI.
        
        Args:
            ci_mode: If True, disable colors and animations
        """
        self.ci_mode = ci_mode
        if ci_mode or not sys.stdout.isatty():
            Colors.disable()
    
    def section(self, title):
        """Print a section header."""
        line = "─" * 60
        print(f"\n{Colors.CYAN}{Colors.BOLD}{line}{Colors.RESET}")
        print(f"{Colors.CYAN}{Colors.BOLD}  {title}{Colors.RESET}")
        print(f"{Colors.CYAN}{Colors.BOLD}{line}{Colors.RESET}")
    
    def subsection(self, title):
        """Print a subsection header."""
        print(f"\n{Colors.BRIGHT_BLUE}{Colors.BOLD}▶ {title}{Colors.RESET}")
    
    def success(self, message):
        """Print a success message."""
        print(f"{Colors.GREEN}[OK]{Colors.RESET} {message}")
    
    def info(self, message):
        """Print an info message."""
        print(f"{Colors.CYAN}[*]{Colors.RESET} {message}")
    
    def warning(self, message):
        """Print a warning message."""
        print(f"{Colors.YELLOW}[!]{Colors.RESET} {message}")
    
    def error(self, message):
        """Print an error message."""
        print(f"{Colors.RED}[ERROR]{Colors.RESET} {message}")
    
    def command(self, cmd):
        """Print a command being executed."""
        cmd_str = " ".join(cmd) if isinstance(cmd, list) else cmd
        print(f"{Colors.DIM}$ {cmd_str}{Colors.RESET}")
    
    def result(self, label, value):
        """Print a key-value result."""
        print(f"{Colors.BRIGHT_WHITE}{label}:{Colors.RESET} {value}")
    
    def banner(self, title):
        """Print a fancy banner."""
        width = 60
        print(f"\n{Colors.BRIGHT_CYAN}{Colors.BOLD}")
        print("╔" + "═" * (width - 2) + "╗")
        padding = (width - 2 - len(title)) // 2
        print("║" + " " * padding + title + " " * (width - 2 - padding - len(title)) + "║")
        print("╚" + "═" * (width - 2) + "╝")
        print(f"{Colors.RESET}")
