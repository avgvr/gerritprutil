// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <vector>

#include <git2.h>

namespace git
{

class Commit
{
private:
    git_commit *commit;
public:
    explicit Commit(git_commit *cmt) noexcept
        : commit(cmt) {};
    Commit(Commit &) = default;
    Commit(Commit &&) = default;
    ~Commit() {if(commit) git_commit_free(commit);}

    std::string summary(){return std::string(git_commit_summary(commit));};
};

class Repository
{
private:
    int validness;
    git_repository *repo = nullptr;

    void handleResult(int result)
    {
        if(result < 0)
        {
            const git_error *e = git_error_last();
            throw std::runtime_error(std::string(e->message)
                + "\n\t With code: "
                + std::to_string(e->klass));
        }
    };

public:
    Repository() noexcept
        : validness(git_libgit2_init())
    {
    };
    explicit Repository(std::string &path) noexcept
        : validness(git_libgit2_init())
    {
        if(validness > -1)
        {
            int exists = git_repository_open_ext(NULL, path.c_str(), 0, NULL);
            if(exists == 0)
            {
                if(git_repository_open(&repo, path.c_str()) < 0)
                    validness = -1, git_libgit2_shutdown();
            }
            else if(exists == GIT_ENOTFOUND)
            {
                git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
                opts.flags = GIT_REPOSITORY_INIT_MKPATH;
                exists = git_repository_init_ext(&repo, path.c_str(), &opts);
                if(exists < 0) validness = -1, git_libgit2_shutdown();
            }
            else validness = -1, git_libgit2_shutdown();
        }
    };
    ~Repository()
    {
        if(repo) git_repository_free(repo);
        if(validness) git_libgit2_shutdown();
    };

    void isValid()
    {
        if(validness < 0)
            throw std::runtime_error("Repository "
                + std::string(git_repository_path(repo))
                + " is not valid");
    }

    void fetchRemote(const std::string url, const std::vector<std::string> refspecs)
    {
        isValid();

        std::vector<const char *> specs(refspecs.size());
        for(int i = 0; i < specs.size(); i++)
        {
            specs[i] = refspecs[i].data();
        }
        git_strarray gitspec = {const_cast<char **>(specs.data()), specs.size()};
        git_remote *remote = NULL;

        handleResult(git_remote_create_anonymous(&remote, repo, url.c_str()));
        handleResult(git_remote_fetch(remote, &gitspec, NULL, NULL));

        git_remote_free(remote);
    };

    std::vector<std::unique_ptr<Commit>> listCommits(const std::string headsha, const std::string backsha)
    {
        isValid();
        std::vector<std::unique_ptr<Commit>> commits;

        std::string range = headsha + ".." + backsha;
        git_revwalk *walker;

        handleResult(git_revwalk_new(&walker, repo));
        handleResult(git_revwalk_sorting(walker, GIT_SORT_TOPOLOGICAL));
        handleResult(git_revwalk_push_range(walker, range.c_str()));

        git_commit *commit;
        git_oid oid;
        handleResult(git_oid_fromstr(&oid, headsha.c_str()));
        handleResult(git_commit_lookup(&commit, repo, &oid));
        commits.push_back(std::move(std::make_unique<Commit>(commit)));

        while(git_revwalk_next(&oid, walker) == 0)
        {
            handleResult(git_commit_lookup(&commit, repo, &oid));
            auto commitptr = std::make_unique<Commit>(commit);
            commits.push_back(std::move(commitptr));
        }

        git_revwalk_free(walker);

        return commits;
    };
};

};
