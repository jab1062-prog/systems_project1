#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

typedef struct
{
    char **av;
    size_t n;
    char *in, *out;
    int append;
} Cmd;




static void *mem(void *p, size_t n)
{
    void *q = realloc(p, n);

    if (!q)
    {
        perror("allocation");
        exit(EXIT_FAILURE);
    }

    return q;
}




static char *copy(const char *s)
{
    char *p = mem(NULL, strlen(s) + 1);

    strcpy(p, s);

    return p;
}




static char *cwd(void)
{
    size_t n = 128;
    char *p = NULL;

    while (1)
    {
        p = mem(p, n);

        if (getcwd(p, n))
        {
            return p;
        }

        if (errno != ERANGE)
        {
            perror("getcwd");
            free(p);
            return NULL;
        }

        n *= 2;
    }
}





static void arg(Cmd *c, const char *s)
{
    c->av = mem(c->av, (c->n + 2) * sizeof(char *));

    c->av[c->n++] = copy(s);
    c->av[c->n] = NULL;
}




static void expand(Cmd *c, const char *s)
{
    const char *star = strchr(s, '*');
    const char *slash = strrchr(s, '/');

    if (!star || (slash && star < slash))
    {
        arg(c, s);
        return;
    }

    
    size_t base = slash ? (size_t)(slash - s + 1) : 0;

    char *dir = copy(s);
    dir[base] = '\0';

    
    DIR *d = opendir(base ? dir : ".");
    free(dir);


    
    size_t before = (size_t)(star - s) - base;
    size_t after = strlen(star + 1);
    size_t old = c->n;



    
    if (d != NULL)
    {
        struct dirent *e;

        while ((e = readdir(d)))
        {
            const char *name = e->d_name;
            size_t len = strlen(name);

            if (s[base] == '*' && name[0] == '.')
            {
                continue;
            }

            if (len < before + after || strncmp(name, s + base, before) || strcmp(name + len - after, star + 1))
            {
                continue;
            }

            
            char *path = mem(NULL, base + len + 1);

            memcpy(path, s, base);
            strcpy(path + base, name);

            arg(c, path);

            free(path);
        }

        closedir(d);
    }

    if (c->n == old)
    {
        arg(c, s);
    }
}




static int special(const char *s)
{
    return !strcmp(s, "|") || !strcmp(s, "<") || !strcmp(s, ">") || !strcmp(s, ">>");
}



static void release(Cmd *c, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = 0; j < c[i].n; j++)
        {
            free(c[i].av[j]);
        }

        free(c[i].av);
        free(c[i].in);
        free(c[i].out);
    }

    free(c);
}




static int parse(char *line, Cmd **result, size_t *count)
{
    char *hash = strchr(line, '#');

    if (hash)
    {
        *hash = '\0';
    }

    size_t n = 1;

    Cmd *c = mem(NULL, sizeof *c);
    memset(c, 0, sizeof *c);

    char *save;
    char *t = strtok_r(line, " \t\r\v\f", &save);

    if (!t)
    {
        free(c);
        *count = 0;
        *result = NULL;
        return 0;
    }

    for (; t; t = strtok_r(NULL, " \t\r\v\f", &save))
    {
        Cmd *cur = &c[n - 1];

        if (!strcmp(t, "|"))
        {
            if (!cur->n)
            {
                goto bad;
            }

            c = mem(c, (n + 1) * sizeof *c);
            memset(&c[n++], 0, sizeof *c);
        }
        else if (special(t))
        {
            int input = !strcmp(t, "<");
            int append = !strcmp(t, ">>");

            char *file = strtok_r(NULL, " \t\r\v\f", &save);

            if (!file || special(file))
            {
                goto bad;
            }

            char **target = input ? &cur->in : &cur->out;

            if (*target)
            {
                goto bad;
            }

            *target = copy(file);

            if (!input)
            {
                cur->append = append;
            }
        }
        else
        {
            expand(cur, t);
        }
    }

    if (!c[n - 1].n)
    {
        goto bad;
    }

    *result = c;
    *count = n;

    return 0;

bad:
    fprintf(stderr, "mysh: syntax error\n");
    release(c, n);

    return 1;
}

static char *resolve(const char *name)
{
    if (strchr(name, '/'))
    {
        return access(name, X_OK) == 0 ? copy(name) : NULL;
    }

    const char *dirs[] =
    {
        "/usr/local/bin",
        "/usr/bin",
        "/bin"
    };

    for (size_t i = 0; i < 3; i++)
    {
        char *p = mem(NULL, strlen(dirs[i]) + strlen(name) + 2);

        sprintf(p, "%s/%s", dirs[i], name);

        if (access(p, X_OK) == 0)
        {
            return p;
        }

        free(p);
    }

    return NULL;
}


static int builtin(Cmd *c)
{
    const char *s = c->av[0];

    return !strcmp(s, "cd") || !strcmp(s, "pwd") || !strcmp(s, "which") || !strcmp(s, "exit");
}


static int run_builtin(Cmd *c)
{
    char **v = c->av;

    if (!strcmp(v[0], "exit"))
    {
        return 0;
    }

    if (!strcmp(v[0], "cd"))
    {
        if (c->n > 2)
        {
            fprintf(stderr, "cd: too many arguments\n");
            return 1;
        }

        const char *p = c->n == 2 ? v[1] : getenv("HOME");

        if (!p)
        {
            fprintf(stderr, "cd: HOME unavailable\n");
            return 1;
        }

        if (chdir(p) < 0)
        {
            perror("cd");
            return 1;
        }

        return 0;
    }

    if (!strcmp(v[0], "pwd"))
    {
        if (c->n != 1)
        {
            fprintf(stderr, "pwd: unexpected arguments\n");
            return 1;
        }

        char *p = cwd();

        if (!p)
        {
            return 1;
        }

        int r = dprintf(1, "%s\n", p) < 0;

        free(p);

        return r;
    }

    int failed = c->n == 1;

    if (failed)
    {
        fprintf(stderr, "which: expected a program name\n");
    }

    for (size_t i = 1; i < c->n; i++)
    {
        char *p = resolve(v[i]);

        if (p)
        {
            if (dprintf(1, "%s\n", p) < 0)
            {
                failed = 1;
            }

            free(p);
        }
        else
        {
            fprintf(stderr, "which: %s: not found\n", v[i]);
            failed = 1;
        }
    }

    return failed;
}

static int redirect(Cmd *c)
{
    for (int i = 0; i < 2; i++)
    {
        const char *p = i ? c->out : c->in;

        if (!p)
        {
            continue;
        }

        int fd = open(p,i ? O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC) : O_RDONLY, 0640);

        if (fd < 0)
        {
            perror(p);
            return 1;
        }

        if (dup2(fd, i) < 0)
        {
            perror("dup2");
            close(fd);
            return 1;
        }

        close(fd);
    }

    return 0;
}



static int execute(Cmd *c, size_t n, int interactive, int input)
{
    if (n == 1 && builtin(c))
    {
        int a = dup(0);
        int b = dup(1);
        int r = 1;

        if (a < 0 || b < 0)
        {
            perror("dup");
        }
        else
        {
            r = redirect(c);

            if (!r)
            {
                r = run_builtin(c);
            }

            if (dup2(a, 0) < 0 || dup2(b, 1) < 0)
            {
                perror("restore");
                exit(EXIT_FAILURE);
            }
        }

        if (a >= 0)
        {
            close(a);
        }

        if (b >= 0)
        {
            close(b);
        }

        return r << 8;
    }

    pid_t *pids = mem(NULL, n * sizeof *pids);

    size_t started = 0;

    int prev = -1;
    int failed = 0;
    int result = 0;

    
    for (size_t i = 0; i < n; i++)
    {
        int fds[2] = {-1, -1};

        if (i + 1 < n && pipe(fds) < 0)
        {
            perror("pipe");
            failed = 1;
            break;
        }

        
        pid_t p = fork();

        if (p < 0)
        {
            perror("fork");

            if (fds[0] >= 0)
            {
                close(fds[0]);
                close(fds[1]);
            }

            failed = 1;
            break;
        }

        if (!p)
        {
            if (input > 2)
            {
                close(input);
            }

            if (prev >= 0)
            {
                if (dup2(prev, 0) < 0)
                {
                    perror("dup2");
                    _exit(1);
                }

                close(prev);
            }

                
            else if (!interactive)
            {
                int fd = open("/dev/null", O_RDONLY);

                if (fd < 0 || dup2(fd, 0) < 0)
                {
                    perror("/dev/null");
                    _exit(1);
                }

                close(fd);
            }

            if (fds[0] >= 0)
            {
                close(fds[0]);

                if (dup2(fds[1], 1) < 0)
                {
                    perror("dup2");
                    _exit(1);
                }

                close(fds[1]);
            }

            if (redirect(&c[i]))
            {
                _exit(1);
            }

            if (builtin(&c[i]))
            {
                _exit(run_builtin(&c[i]));
            }

            char *path = resolve(c[i].av[0]);

            if (!path)
            {
                fprintf(stderr, "%s: not found\n", c[i].av[0]); _exit(1);
            }

            execv(path, c[i].av);

            perror(path);
            free(path);

            _exit(1);
        }


        
        pids[started++] = p;


        
        
        if (prev >= 0)
        {
            close(prev);
        }

        if (fds[1] >= 0)
        {
            close(fds[1]);
        }

        prev = fds[0];
    }


    
    if (prev >= 0)
    {
        close(prev);
    }


    if (failed)
    {
        for (size_t i = 0; i < started; i++)
        {
            kill(pids[i], SIGKILL);
        }
    }


    
    for (size_t i = 0; i < started; i++)
    {
        int status;
        pid_t r;

        do
        {
            r = waitpid(pids[i], &status, 0);
        }
        while (r < 0 && errno == EINTR);

        if (r < 0)
        {
            perror("waitpid");
            failed = 1;
        }
        else if (i == n - 1)
        {
            result = status;
        }
    }

    free(pids);

    return failed ? 1 << 8 : result;
}


static void prompt(void)
{
    char *p = cwd();
    const char *h = getenv("HOME");

    if (!p)
    {
        dprintf(1, "?$ ");
        return;
    }

    size_t len = h ? strlen(h) : 0;

    while (len > 1 && h[len - 1] == '/')
    {
        len--;
    }

    if (len && !strncmp(p, h, len) && (p[len] == 0 || p[len] == '/' || len == 1))
    {
        dprintf(1, "~%s%s$ ", len == 1 && p[1] ? "/" : "", p + len);
    }
    else
    {
        dprintf(1, "%s$ ", p);
    }

    free(p);
}



int main(int argc, char **argv)
{
    if (argc > 2)
    {
        fprintf(stderr, "usage: mysh [file]\n");
        return 0;
    }


    
    int fd = argc == 2 ? open(argv[1], O_RDONLY) : 0;

    if (fd < 0)
    {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    int interactive = argc == 1 && isatty(0);
    int stop = 0;


    
    if (interactive)
    {
        dprintf(1, "Welcome to my shell!\n");
    }

    while (!stop)
    {
        if (interactive)
        {
            prompt();
        }

        char *line = NULL;

        size_t len = 0;
        size_t cap = 0;

        char ch;
        ssize_t r;


        
        for (;;)
        {
            do
            {
                r = read(fd, &ch, 1);
            }
            while (r < 0 && errno == EINTR);

            if (r <= 0 || ch == '\n')
            {
                break;
            }

            if (len + 1 >= cap)
            {
                cap = cap ? cap * 2 : 128;
                line = mem(line, cap);
            }

            line[len++] = ch;
        }


        
        if (r < 0)
        {
            perror("read");
            free(line);
            break;
        }

        

        if (r == 0 && !len)
        {
            free(line);
            break;
        }


        
        line = mem(line, len + 1);
        line[len] = '\0';


        
        Cmd *c = NULL;
        size_t n = 0;


        
        int status = 1 << 8;


        
        if (!parse(line, &c, &n))
        {
            for (size_t i = 0; i < n; i++)
            {
                if (!strcmp(c[i].av[0], "exit"))
                {
                    stop = 1;
                }
            }

            if (n > 0)
            {
                status = execute(c, n, interactive, fd);
            }
            else
            {
                status = 0;
            }

            release(c, n);
        }

        free(line);

        if (interactive && WIFSIGNALED(status))
        {
            dprintf(1, "Terminated by signal %s\n", strsignal(WTERMSIG(status)));
        }
        else if (interactive && WIFEXITED(status) && WEXITSTATUS(status))
        {
            dprintf(1, "Exited with status %d\n", WEXITSTATUS(status));
        }

        if (r == 0)
        {
            break;
        }
    }

    if (fd != 0)
    {
        close(fd);
    }

    if (interactive)
    {
        dprintf(1, "Exiting my shell.\n");
    }

    return EXIT_SUCCESS;
}
