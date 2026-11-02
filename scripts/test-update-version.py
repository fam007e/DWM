#!/usr/bin/env python3
import importlib.machinery
import importlib.util
from pathlib import Path
import subprocess
import tempfile


loader = importlib.machinery.SourceFileLoader(
    "dwm_desktop_update", str(Path(__file__).with_name("dwm-desktop-update")))
spec = importlib.util.spec_from_loader(loader.name, loader)
update = importlib.util.module_from_spec(spec)
loader.exec_module(update)


def git(repo, *args):
    return subprocess.check_output(["git", *args], cwd=repo, text=True, stderr=subprocess.DEVNULL).strip()


def commit(repo, message):
    (repo / "tracked").write_text(message)
    git(repo, "add", "tracked")
    git(repo, "commit", "-m", message)
    return git(repo, "rev-parse", "HEAD")


with tempfile.TemporaryDirectory() as temporary:
    tagged_repo = Path(temporary) / "tagged"
    tagged_repo.mkdir()
    git(tagged_repo, "init", "-q")
    git(tagged_repo, "config", "user.name", "Test")
    git(tagged_repo, "config", "user.email", "test@example.com")
    tagged_revision = commit(tagged_repo, "release")
    git(tagged_repo, "tag", "-a", "v2026.11.02", "-m", "release")
    assert update.source_build_version(tagged_repo, tagged_revision) == "2026.11.02"

    fetched_repo = Path(temporary) / "fetched"
    fetched_repo.mkdir()
    git(fetched_repo, "init", "-q")
    git(fetched_repo, "fetch", "--no-tags", str(tagged_repo), tagged_revision)
    git(fetched_repo, "checkout", "--detach", "FETCH_HEAD")
    git(fetched_repo, "fetch", "--no-tags", str(tagged_repo), "+refs/tags/v*:refs/tags/v*")
    assert update.source_build_version(fetched_repo, tagged_revision) == "2026.11.02"

    next_revision = commit(tagged_repo, "after release")
    assert update.source_build_version(tagged_repo, next_revision) == "2026.11.02-1-g" + next_revision[:7]

    untagged_repo = Path(temporary) / "untagged"
    untagged_repo.mkdir()
    git(untagged_repo, "init", "-q")
    git(untagged_repo, "config", "user.name", "Test")
    git(untagged_repo, "config", "user.email", "test@example.com")
    untagged_revision = commit(untagged_repo, "untagged")
    assert update.source_build_version(untagged_repo, untagged_revision) == "git-" + untagged_revision[:7]

print("Source build version tests passed.")
