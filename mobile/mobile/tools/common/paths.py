import os


self_dir = os.path.dirname(os.path.abspath(__file__))
wam_root = os.path.abspath(os.path.join(self_dir, os.pardir, os.pardir))
repo_root = os.path.abspath(os.path.join(wam_root, os.pardir, os.pardir))
chromium_root = os.path.abspath(os.path.join(
    wam_root, os.pardir, os.pardir, 'chromium', 'src'))
