/*
 * Copyright (C) 2019-2021 Data Ductus AB
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#endif

#include "env.h"

struct FileDescriptorData fd_data[MAX_FD];
int wakeup_pipe[2];

extern char rts_exit;
extern int return_val;

#ifdef IS_MACOS         // Use kqueue
int kq;
void EVENT_init() {
    kq = kqueue();
    struct kevent wakeup;
    EV_SET(&wakeup, wakeup_pipe[0], EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &wakeup, 1, NULL, 0, NULL);
}
void EVENT_add_read(int fd) {
    EV_SET(&fd_data[fd].event_spec, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &fd_data[fd].event_spec, 1, NULL, 0, NULL);
}
void EVENT_add_read_once(int fd) {
    EV_SET(&fd_data[fd].event_spec, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, NULL);
    kevent(kq, &fd_data[fd].event_spec, 1, NULL, 0, NULL);
}
void EVENT_mod_read_once(int fd) {
    EV_SET(&fd_data[fd].event_spec, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, NULL);
    kevent(kq, &fd_data[fd].event_spec, 1, NULL, 0, NULL);
}
void EVENT_add_write_once(int fd) {
    EV_SET(&fd_data[fd].event_spec, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
    kevent(kq, &fd_data[fd].event_spec, 1, NULL, 0, NULL);
}
void EVENT_del_read(int fd) {
    EV_SET(&fd_data[fd].event_spec, fd, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
    kevent(kq, &fd_data[fd].event_spec, 1, NULL, 0, NULL);
}
int EVENT_wait(EVENT_type *ev, struct timespec *timeout) {
    return kevent(kq, NULL, 0, ev, 1, timeout);
}
int EVENT_fd(EVENT_type *ev) {
    return ev->ident;
}
int EVENT_is_wakeup(EVENT_type *ev) {
    return ev->filter == EVFILT_READ & ev->ident == wakeup_pipe[0];
}
int EVENT_is_eof(EVENT_type *ev) {
    return ev->flags & EV_EOF;
}
int EVENT_is_error(EVENT_type *ev) {
    return ev->flags & EV_ERROR;
}
int EVENT_errno(EVENT_type *ev) {
    return ev->data;
}
int EVENT_is_read(EVENT_type *ev) {
    return ev->filter==EVFILT_READ;
}
int EVENT_fd_is_read(int fd) {
    return fd_data[fd].event_spec.filter == EVFILT_READ;
}
#endif

#ifdef IS_GNU_LINUX             // Use epoll            
int ep;
void EVENT_init() {
    ep = epoll_create(1);
    struct epoll_event wakeup;
    wakeup.events = EPOLLIN;
    wakeup.data.fd = wakeup_pipe[0];
    epoll_ctl(ep, EPOLL_CTL_ADD, wakeup_pipe[0], &wakeup);
}
void EVENT_add_read(int fd) {
    fd_data[fd].event_spec.events = EPOLLIN | EPOLLRDHUP;
    fd_data[fd].event_spec.data.fd = fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, fd, &fd_data[fd].event_spec);
}
void EVENT_add_read_once(int fd) {
    fd_data[fd].event_spec.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
    fd_data[fd].event_spec.data.fd = fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, fd, &fd_data[fd].event_spec);
}
void EVENT_mod_read_once(int fd) {
    fd_data[fd].event_spec.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
    fd_data[fd].event_spec.data.fd = fd;
    epoll_ctl(ep, EPOLL_CTL_MOD, fd, &fd_data[fd].event_spec);
}
void EVENT_add_write_once(int fd) {
    fd_data[fd].event_spec.events = EPOLLOUT | EPOLLRDHUP | EPOLLONESHOT;
    fd_data[fd].event_spec.data.fd = fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, fd, &fd_data[fd].event_spec);
}
void EVENT_del_read(int fd) {
    fd_data[fd].event_spec.events = EPOLLIN;
    fd_data[fd].event_spec.data.fd = fd;
    epoll_ctl(ep, EPOLL_CTL_DEL, fd, &fd_data[fd].event_spec);
}
int EVENT_wait(EVENT_type *ev, struct timespec *timeout) {
    int msec = timeout ? timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000 : -1;
    return epoll_wait(ep, ev, 1, msec);
//    return epoll_pwait2(ep, ev, 1, timeout, NULL);        // appears in linux kernel 5.11
}
int EVENT_fd(EVENT_type *ev) {
    return ev->data.fd;
}
int EVENT_is_wakeup(EVENT_type *ev) {
    return (ev->events & EPOLLIN) && ev->data.fd == wakeup_pipe[0];
}
// epoll TCP disconnect is EPOLLRDHUP (remote bla hup) and various other socket
// errors (like what?) go in EPOLLHUP so we check for both
int EVENT_is_eof(EVENT_type *ev) {
    return ev->events & (EPOLLHUP | EPOLLRDHUP);
}
int EVENT_is_error(EVENT_type *ev) {
    return ev->events & EPOLLERR;
}
int EVENT_errno(EVENT_type *ev) {
    int error = 0;
    socklen_t errlen = sizeof(error);
    getsockopt(ev->data.fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
    return error;
}
int EVENT_is_read(EVENT_type *ev) {
    return ev->events & EPOLLIN;
}
int EVENT_fd_is_read(int fd) {
    return fd_data[fd].event_spec.events & EPOLLIN;
}
#endif

static void $init_FileDescriptorData(int fd) {
  fd_data[fd].kind = nohandler;
  bzero(fd_data[fd].buffer,BUF_SIZE);
}

int new_socket ($function handler) {
  int fd = socket(PF_INET,SOCK_STREAM,0);
  fcntl(fd,F_SETFL,O_NONBLOCK);
  fd_data[fd].kind = connecthandler;
  fd_data[fd].chandler = handler;
  return fd;
}

void setupConnection (int fd) {
  $Connection conn = $NEW($Connection,fd);
  fd_data[fd].conn = conn;
  fd_data[fd].chandler->$class->__call__(fd_data[fd].chandler, conn);
}

$str $getName(int fd) {
  socklen_t socklen = sizeof(struct sockaddr_in);
  char *buf = malloc(100);
  getnameinfo((struct sockaddr *)&fd_data[fd].sock_addr,socklen,buf,100,NULL,0,0);
  return to$str(buf);
}

///////////////////////////////////////////////////////////////////////////////////////////

$NoneType env$$l$1lambda$__init__ (env$$l$1lambda p$self, $Env __self__, $str s) {
    p$self->__self__ = __self__;
    p$self->s = s;
    return $None;
}
$R env$$l$1lambda$__call__ (env$$l$1lambda p$self, $Cont c$cont) {
    $Env __self__ = p$self->__self__;
    $str s = p$self->s;
    return __self__->$class->stdout_write$local(__self__, s, c$cont);
}
void env$$l$1lambda$__serialize__ (env$$l$1lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->s, state);
}
env$$l$1lambda env$$l$1lambda$__deserialize__ (env$$l$1lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$1lambda));
            self->$class = &env$$l$1lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$1lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->s = $step_deserialize(state);
    return self;
}
env$$l$1lambda env$$l$1lambda$new($Env p$1, $str p$2) {
    env$$l$1lambda $tmp = malloc(sizeof(struct env$$l$1lambda));
    $tmp->$class = &env$$l$1lambda$methods;
    env$$l$1lambda$methods.__init__($tmp, p$1, p$2);
    return $tmp;
}
struct env$$l$1lambda$class env$$l$1lambda$methods;
$NoneType env$$l$2lambda$__init__ (env$$l$2lambda p$self, $Env __self__, $function cb) {
    p$self->__self__ = __self__;
    p$self->cb = cb;
    return $None;
}
$R env$$l$2lambda$__call__ (env$$l$2lambda p$self, $Cont c$cont) {
    $Env __self__ = p$self->__self__;
    $function cb = p$self->cb;
    return __self__->$class->stdin_install$local(__self__, cb, c$cont);
}
void env$$l$2lambda$__serialize__ (env$$l$2lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->cb, state);
}
env$$l$2lambda env$$l$2lambda$__deserialize__ (env$$l$2lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$2lambda));
            self->$class = &env$$l$2lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$2lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->cb = $step_deserialize(state);
    return self;
}
env$$l$2lambda env$$l$2lambda$new($Env p$1, $function p$2) {
    env$$l$2lambda $tmp = malloc(sizeof(struct env$$l$2lambda));
    $tmp->$class = &env$$l$2lambda$methods;
    env$$l$2lambda$methods.__init__($tmp, p$1, p$2);
    return $tmp;
}
struct env$$l$2lambda$class env$$l$2lambda$methods;
$NoneType env$$l$3lambda$__init__ (env$$l$3lambda p$self, $Env __self__, $str host, $int port, $function cb) {
    p$self->__self__ = __self__;
    p$self->host = host;
    p$self->port = port;
    p$self->cb = cb;
    return $None;
}
$R env$$l$3lambda$__call__ (env$$l$3lambda p$self, $Cont c$cont) {
    $Env __self__ = p$self->__self__;
    $str host = p$self->host;
    $int port = p$self->port;
    $function cb = p$self->cb;
    return __self__->$class->connect$local(__self__, host, port, cb, c$cont);
}
void env$$l$3lambda$__serialize__ (env$$l$3lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->host, state);
    $step_serialize(self->port, state);
    $step_serialize(self->cb, state);
}
env$$l$3lambda env$$l$3lambda$__deserialize__ (env$$l$3lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$3lambda));
            self->$class = &env$$l$3lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$3lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->host = $step_deserialize(state);
    self->port = $step_deserialize(state);
    self->cb = $step_deserialize(state);
    return self;
}
env$$l$3lambda env$$l$3lambda$new($Env p$1, $str p$2, $int p$3, $function p$4) {
    env$$l$3lambda $tmp = malloc(sizeof(struct env$$l$3lambda));
    $tmp->$class = &env$$l$3lambda$methods;
    env$$l$3lambda$methods.__init__($tmp, p$1, p$2, p$3, p$4);
    return $tmp;
}
struct env$$l$3lambda$class env$$l$3lambda$methods;
$NoneType env$$l$4lambda$__init__ (env$$l$4lambda p$self, $Env __self__, $int port, $function cb) {
    p$self->__self__ = __self__;
    p$self->port = port;
    p$self->cb = cb;
    return $None;
}
$R env$$l$4lambda$__call__ (env$$l$4lambda p$self, $Cont c$cont) {
    $Env __self__ = p$self->__self__;
    $int port = p$self->port;
    $function cb = p$self->cb;
    return __self__->$class->listen$local(__self__, port, cb, c$cont);
}
void env$$l$4lambda$__serialize__ (env$$l$4lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->port, state);
    $step_serialize(self->cb, state);
}
env$$l$4lambda env$$l$4lambda$__deserialize__ (env$$l$4lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$4lambda));
            self->$class = &env$$l$4lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$4lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->port = $step_deserialize(state);
    self->cb = $step_deserialize(state);
    return self;
}
env$$l$4lambda env$$l$4lambda$new($Env p$1, $int p$2, $function p$3) {
    env$$l$4lambda $tmp = malloc(sizeof(struct env$$l$4lambda));
    $tmp->$class = &env$$l$4lambda$methods;
    env$$l$4lambda$methods.__init__($tmp, p$1, p$2, p$3);
    return $tmp;
}
struct env$$l$4lambda$class env$$l$4lambda$methods;
$NoneType env$$l$5lambda$__init__ (env$$l$5lambda p$self, $Env __self__, $int n) {
    p$self->__self__ = __self__;
    p$self->n = n;
    return $None;
}
$R env$$l$5lambda$__call__ (env$$l$5lambda p$self, $Cont c$cont) {
    $Env __self__ = p$self->__self__;
    $int n = p$self->n;
    return __self__->$class->exit$local(__self__, n, c$cont);
}
void env$$l$5lambda$__serialize__ (env$$l$5lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->n, state);
}
env$$l$5lambda env$$l$5lambda$__deserialize__ (env$$l$5lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$5lambda));
            self->$class = &env$$l$5lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$5lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->n = $step_deserialize(state);
    return self;
}
env$$l$5lambda env$$l$5lambda$new($Env p$1, $int p$2) {
    env$$l$5lambda $tmp = malloc(sizeof(struct env$$l$5lambda));
    $tmp->$class = &env$$l$5lambda$methods;
    env$$l$5lambda$methods.__init__($tmp, p$1, p$2);
    return $tmp;
}
struct env$$l$5lambda$class env$$l$5lambda$methods;
$NoneType env$$l$6lambda$__init__ (env$$l$6lambda p$self, $Env __self__, $str nm) {
    p$self->__self__ = __self__;
    p$self->nm = nm;
    return $None;
}
$R env$$l$6lambda$__call__ (env$$l$6lambda p$self, $Cont c$cont) {
    $Env __self__ = p$self->__self__;
    $str nm = p$self->nm;
    return __self__->$class->openR$local(__self__, nm, c$cont);
}
void env$$l$6lambda$__serialize__ (env$$l$6lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->nm, state);
}
env$$l$6lambda env$$l$6lambda$__deserialize__ (env$$l$6lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$6lambda));
            self->$class = &env$$l$6lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$6lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->nm = $step_deserialize(state);
    return self;
}
env$$l$6lambda env$$l$6lambda$new($Env p$1, $str p$2) {
    env$$l$6lambda $tmp = malloc(sizeof(struct env$$l$6lambda));
    $tmp->$class = &env$$l$6lambda$methods;
    env$$l$6lambda$methods.__init__($tmp, p$1, p$2);
    return $tmp;
}
struct env$$l$6lambda$class env$$l$6lambda$methods;
$NoneType env$$l$7lambda$__init__ (env$$l$7lambda p$self, $Env __self__, $str nm) {
    p$self->__self__ = __self__;
    p$self->nm = nm;
    return $None;
}
$R env$$l$7lambda$__call__ (env$$l$7lambda p$self, $Cont c$cont) {
    $Env __self__ = p$self->__self__;
    $str nm = p$self->nm;
    return __self__->$class->openW$local(__self__, nm, c$cont);
}
void env$$l$7lambda$__serialize__ (env$$l$7lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->nm, state);
}
env$$l$7lambda env$$l$7lambda$__deserialize__ (env$$l$7lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$7lambda));
            self->$class = &env$$l$7lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$7lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->nm = $step_deserialize(state);
    return self;
}
env$$l$7lambda env$$l$7lambda$new($Env p$1, $str p$2) {
    env$$l$7lambda $tmp = malloc(sizeof(struct env$$l$7lambda));
    $tmp->$class = &env$$l$7lambda$methods;
    env$$l$7lambda$methods.__init__($tmp, p$1, p$2);
    return $tmp;
}
struct env$$l$7lambda$class env$$l$7lambda$methods;
$NoneType env$$l$8lambda$__init__ (env$$l$8lambda p$self, $Connection __self__, $str s) {
    p$self->__self__ = __self__;
    p$self->s = s;
    return $None;
}
$R env$$l$8lambda$__call__ (env$$l$8lambda p$self, $Cont c$cont) {
    $Connection __self__ = p$self->__self__;
    $str s = p$self->s;
    return __self__->$class->write$local(__self__, s, c$cont);
}
void env$$l$8lambda$__serialize__ (env$$l$8lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->s, state);
}
env$$l$8lambda env$$l$8lambda$__deserialize__ (env$$l$8lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$8lambda));
            self->$class = &env$$l$8lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$8lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->s = $step_deserialize(state);
    return self;
}
env$$l$8lambda env$$l$8lambda$new($Connection p$1, $str p$2) {
    env$$l$8lambda $tmp = malloc(sizeof(struct env$$l$8lambda));
    $tmp->$class = &env$$l$8lambda$methods;
    env$$l$8lambda$methods.__init__($tmp, p$1, p$2);
    return $tmp;
}
struct env$$l$8lambda$class env$$l$8lambda$methods;
$NoneType env$$l$9lambda$__init__ (env$$l$9lambda p$self, $Connection __self__) {
    p$self->__self__ = __self__;
    return $None;
}
$R env$$l$9lambda$__call__ (env$$l$9lambda p$self, $Cont c$cont) {
    $Connection __self__ = p$self->__self__;
    return __self__->$class->close$local(__self__, c$cont);
}
void env$$l$9lambda$__serialize__ (env$$l$9lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
}
env$$l$9lambda env$$l$9lambda$__deserialize__ (env$$l$9lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$9lambda));
            self->$class = &env$$l$9lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$9lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    return self;
}
env$$l$9lambda env$$l$9lambda$new($Connection p$1) {
    env$$l$9lambda $tmp = malloc(sizeof(struct env$$l$9lambda));
    $tmp->$class = &env$$l$9lambda$methods;
    env$$l$9lambda$methods.__init__($tmp, p$1);
    return $tmp;
}
struct env$$l$9lambda$class env$$l$9lambda$methods;
$NoneType env$$l$10lambda$__init__ (env$$l$10lambda p$self, $Connection __self__, $function cb1, $function cb2) {
    p$self->__self__ = __self__;
    p$self->cb1 = cb1;
    p$self->cb2 = cb2;
    return $None;
}
$R env$$l$10lambda$__call__ (env$$l$10lambda p$self, $Cont c$cont) {
    $Connection __self__ = p$self->__self__;
    $function cb1 = p$self->cb1;
    $function cb2 = p$self->cb2;
    return __self__->$class->on_receipt$local(__self__, cb1, cb2, c$cont);
}
void env$$l$10lambda$__serialize__ (env$$l$10lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->cb1, state);
    $step_serialize(self->cb2, state);
}
env$$l$10lambda env$$l$10lambda$__deserialize__ (env$$l$10lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$10lambda));
            self->$class = &env$$l$10lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$10lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->cb1 = $step_deserialize(state);
    self->cb2 = $step_deserialize(state);
    return self;
}
env$$l$10lambda env$$l$10lambda$new($Connection p$1, $function p$2, $function p$3) {
    env$$l$10lambda $tmp = malloc(sizeof(struct env$$l$10lambda));
    $tmp->$class = &env$$l$10lambda$methods;
    env$$l$10lambda$methods.__init__($tmp, p$1, p$2, p$3);
    return $tmp;
}
struct env$$l$10lambda$class env$$l$10lambda$methods;
$NoneType env$$l$11lambda$__init__ (env$$l$11lambda p$self, $RFile __self__) {
    p$self->__self__ = __self__;
    return $None;
}
$R env$$l$11lambda$__call__ (env$$l$11lambda p$self, $Cont c$cont) {
    $RFile __self__ = p$self->__self__;
    return __self__->$class->readln$local(__self__, c$cont);
}
void env$$l$11lambda$__serialize__ (env$$l$11lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
}
env$$l$11lambda env$$l$11lambda$__deserialize__ (env$$l$11lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$11lambda));
            self->$class = &env$$l$11lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$11lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    return self;
}
env$$l$11lambda env$$l$11lambda$new($RFile p$1) {
    env$$l$11lambda $tmp = malloc(sizeof(struct env$$l$11lambda));
    $tmp->$class = &env$$l$11lambda$methods;
    env$$l$11lambda$methods.__init__($tmp, p$1);
    return $tmp;
}
struct env$$l$11lambda$class env$$l$11lambda$methods;
$NoneType env$$l$12lambda$__init__ (env$$l$12lambda p$self, $RFile __self__) {
    p$self->__self__ = __self__;
    return $None;
}
$R env$$l$12lambda$__call__ (env$$l$12lambda p$self, $Cont c$cont) {
    $RFile __self__ = p$self->__self__;
    return __self__->$class->close$local(__self__, c$cont);
}
void env$$l$12lambda$__serialize__ (env$$l$12lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
}
env$$l$12lambda env$$l$12lambda$__deserialize__ (env$$l$12lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$12lambda));
            self->$class = &env$$l$12lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$12lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    return self;
}
env$$l$12lambda env$$l$12lambda$new($RFile p$1) {
    env$$l$12lambda $tmp = malloc(sizeof(struct env$$l$12lambda));
    $tmp->$class = &env$$l$12lambda$methods;
    env$$l$12lambda$methods.__init__($tmp, p$1);
    return $tmp;
}
struct env$$l$12lambda$class env$$l$12lambda$methods;
$NoneType env$$l$13lambda$__init__ (env$$l$13lambda p$self, $WFile __self__, $str s) {
    p$self->__self__ = __self__;
    p$self->s = s;
    return $None;
}
$R env$$l$13lambda$__call__ (env$$l$13lambda p$self, $Cont c$cont) {
    $WFile __self__ = p$self->__self__;
    $str s = p$self->s;
    return __self__->$class->write$local(__self__, s, c$cont);
}
void env$$l$13lambda$__serialize__ (env$$l$13lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
    $step_serialize(self->s, state);
}
env$$l$13lambda env$$l$13lambda$__deserialize__ (env$$l$13lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$13lambda));
            self->$class = &env$$l$13lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$13lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    self->s = $step_deserialize(state);
    return self;
}
env$$l$13lambda env$$l$13lambda$new($WFile p$1, $str p$2) {
    env$$l$13lambda $tmp = malloc(sizeof(struct env$$l$13lambda));
    $tmp->$class = &env$$l$13lambda$methods;
    env$$l$13lambda$methods.__init__($tmp, p$1, p$2);
    return $tmp;
}
struct env$$l$13lambda$class env$$l$13lambda$methods;
$NoneType env$$l$14lambda$__init__ (env$$l$14lambda p$self, $WFile __self__) {
    p$self->__self__ = __self__;
    return $None;
}
$R env$$l$14lambda$__call__ (env$$l$14lambda p$self, $Cont c$cont) {
    $WFile __self__ = p$self->__self__;
    return __self__->$class->close$local(__self__, c$cont);
}
void env$$l$14lambda$__serialize__ (env$$l$14lambda self, $Serial$state state) {
    $step_serialize(self->__self__, state);
}
env$$l$14lambda env$$l$14lambda$__deserialize__ (env$$l$14lambda self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct env$$l$14lambda));
            self->$class = &env$$l$14lambda$methods;
            return self;
        }
        self = $DNEW(env$$l$14lambda, state);
    }
    self->__self__ = $step_deserialize(state);
    return self;
}
env$$l$14lambda env$$l$14lambda$new($WFile p$1) {
    env$$l$14lambda $tmp = malloc(sizeof(struct env$$l$14lambda));
    $tmp->$class = &env$$l$14lambda$methods;
    env$$l$14lambda$methods.__init__($tmp, p$1);
    return $tmp;
}
struct env$$l$14lambda$class env$$l$14lambda$methods;
$NoneType $Env$__init__ ($Env __self__, $list argv) {
    __self__->argv = argv;
    return $None;
}
$R $Env$stdout_write$local ($Env __self__, $str s, $Cont c$cont) {
    printf("%s",s->str);
    return $R_CONT(c$cont, $None);
}
$R $Env$stdin_install$local ($Env __self__, $function cb, $Cont c$cont) {
    fd_data[STDIN_FILENO].kind = readhandler;
    fd_data[STDIN_FILENO].rhandler = cb;
    EVENT_add_read(STDIN_FILENO);
    return $R_CONT(c$cont, $None);
}
$R $Env$connect$local ($Env __self__, $str host, $int port, $function cb, $Cont c$cont) {
    struct sockaddr_in addr;
    struct in_addr iaddr;
    struct hostent *ent;
    int hostid;
    int fd = new_socket(cb);
    ent = gethostbyname((char *)host->str); //this should be replaced by calling getaddrinfo
    if(ent==NULL) {
      fd_data[fd].chandler->$class->__call__(fd_data[fd].chandler, NULL);
      //fprintf(stderr,"Name lookup error"); 
    }
    else {
      memcpy(&hostid, ent->h_addr_list[0], sizeof hostid);
      iaddr.s_addr = hostid;
      fd_data[fd].sock_addr.sin_addr = iaddr;
      fd_data[fd].sock_addr.sin_port = htons(port->val);
      fd_data[fd].sock_addr.sin_family = AF_INET;
      if (connect(fd,(struct sockaddr *)&fd_data[fd].sock_addr,sizeof(struct sockaddr)) < 0) { // couldn't connect immediately, 
        if (errno==EINPROGRESS)  {                                                             // so check if attempt continues asynchronously.
          EVENT_add_write_once(fd);
        } else {
          fd_data[fd].chandler->$class->__call__(fd_data[fd].chandler, NULL);
          //fprintf(stderr,"Connect failed");
         }
      } else // connect succeeded immediately (can this ever happen for a non-blocking socket?)
        setupConnection(fd);
    }
    return $R_CONT(c$cont, $None);
}
$R $Env$listen$local ($Env __self__, $int port, $function cb, $Cont c$cont) {
    struct sockaddr_in addr;
    int fd = new_socket(cb);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port->val);
    addr.sin_family = AF_INET;
    if (bind(fd,(struct sockaddr *)&addr,sizeof(struct sockaddr)) < 0)
      fd_data[fd].chandler->$class->__call__(fd_data[fd].chandler, NULL);
    listen(fd,5);
    EVENT_add_read_once(fd);
    return $R_CONT(c$cont, $None);
}
$R $Env$exit$local ($Env __self__, $int n, $Cont c$cont) {
    return_val = n->val;
    rts_exit = 1;
    return $R_CONT(c$cont, $None);
}
$R $Env$openR$local ($Env __self__, $str nm, $Cont c$cont) {
    FILE *file = fopen((char *)nm->str,"r");
    if (file)
        return $R_CONT(c$cont, $RFile$newact(file));
    else
        return $R_CONT(c$cont, $None);
}
$R $Env$openW$local ($Env __self__, $str nm, $Cont c$cont) {
    int descr = open((char *)nm->str, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
    if (descr < 0)
        return $R_CONT(c$cont, $None);
    else
        return $R_CONT(c$cont, $WFile$newact(descr));
}
$Msg $Env$stdout_write ($Env __self__, $str s) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$1lambda$new(__self__, s)));
}
$Msg $Env$stdin_install ($Env __self__, $function cb) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$2lambda$new(__self__, cb)));
}
$Msg $Env$connect ($Env __self__, $str host, $int port, $function cb) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$3lambda$new(__self__, host, port, cb)));
}
$Msg $Env$listen ($Env __self__, $int port, $function cb) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$4lambda$new(__self__, port, cb)));
}
$Msg $Env$exit ($Env __self__, $int n) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$5lambda$new(__self__, n)));
}
$Msg $Env$openR ($Env __self__, $str nm) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$6lambda$new(__self__, nm)));
}
$Msg $Env$openW ($Env __self__, $str nm) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$7lambda$new(__self__, nm)));
}
void $Env$__serialize__ ($Env self, $Serial$state state) {
    $Actor$methods.__serialize__(($Actor)self, state);
    $step_serialize(self->argv, state);
}
$Env $Env$__deserialize__ ($Env self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct $Env));
            self->$class = &$Env$methods;
            return self;
        }
        self = $DNEW($Env, state);
    }
    $Actor$methods.__deserialize__(($Actor)self, state);
    self->argv = $step_deserialize(state);
    return self;
}
$Env $Env$newact($list p$1) {
    $Env $tmp = $NEWACTOR($Env);
    $tmp->$class->__init__($tmp, p$1);  // Inline this message, note that $Env$__init__ is *not* CPS'ed
    return $tmp;
}
struct $Env$class $Env$methods;
$NoneType $Connection$__init__ ($Connection __self__, int descr) {
    __self__->descriptor = descr;
    return $None;
}
$R $Connection$write$local ($Connection __self__, $str s, $Cont c$cont) {
    memcpy(fd_data[__self__->descriptor].buffer,s->str,s->nbytes+1);
    int chunk_size = s->nbytes > BUF_SIZE ? BUF_SIZE : s->nbytes; 
    int r = write(__self__->descriptor,fd_data[__self__->descriptor].buffer,chunk_size);
    //  for now, assume str->nbytes < BUF_SIZE
    return $R_CONT(c$cont, $None);
}
$R $Connection$close$local ($Connection __self__, $Cont c$cont) {
    close(__self__->descriptor); 
    $init_FileDescriptorData(__self__->descriptor);
    return $R_CONT(c$cont, $None);
}
$R $Connection$on_receipt$local ($Connection __self__, $function cb1, $function cb2, $Cont c$cont) {
    fd_data[__self__->descriptor].kind = readhandler;
    fd_data[__self__->descriptor].rhandler = cb1;
    fd_data[__self__->descriptor].errhandler = cb2;
    EVENT_add_read(__self__->descriptor);
    return $R_CONT(c$cont, $None);
}
$Msg $Connection$write ($Connection __self__, $str s) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$8lambda$new(__self__, s)));
}
$Msg $Connection$close ($Connection __self__) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$9lambda$new(__self__)));
}
$Msg $Connection$on_receipt ($Connection __self__, $function cb1, $function cb2) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$10lambda$new(__self__, cb1, cb2)));
}
void $Connection$__serialize__ ($Connection self, $Serial$state state) {
    $Actor$methods.__serialize__(($Actor)self, state);
}
$Connection $Connection$__deserialize__ ($Connection self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct $Connection));
            self->$class = &$Connection$methods;
            return self;
        }
        self = $DNEW($Connection, state);
    }
    $Actor$methods.__deserialize__(($Actor)self, state);
    return self;
}
$Connection $Connection$newact(int descr) {
    $Connection $tmp = $NEWACTOR($Connection);
    $tmp->$class->__init__($tmp, descr);          // Inline this message, note that $Connection$__init__ is *not* CPS'ed
    return $tmp;
}
struct $Connection$class $Connection$methods;
$NoneType $RFile$__init__ ($RFile __self__, FILE *file) {
    __self__->file = file;
    return $None;
}
$R $RFile$readln$local ($RFile __self__, $Cont c$cont) {
    char buf[BUF_SIZE];
    char *res = fgets(buf, BUF_SIZE, __self__->file);
    if (res)
       return $R_CONT(c$cont, to$str(res));
    else
      return $R_CONT(c$cont, $None);      
}                  
$R $RFile$close$local ($RFile __self__, $Cont c$cont) {
    fclose(__self__->file); 
    return $R_CONT(c$cont, $None);
}
$Msg $RFile$readln ($RFile __self__) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$11lambda$new(__self__)));
}
$Msg $RFile$close ($RFile __self__) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$12lambda$new(__self__)));
}
void $RFile$__serialize__ ($RFile self, $Serial$state state) {
    $Actor$methods.__serialize__(($Actor)self, state);
}
$RFile $RFile$__deserialize__ ($RFile self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct $RFile));
            self->$class = &$RFile$methods;
            return self;
        }
        self = $DNEW($RFile, state);
    }
    $Actor$methods.__deserialize__(($Actor)self, state);
    return self;
}
$RFile $RFile$newact(FILE *file) {
    $RFile $tmp = $NEWACTOR($RFile);
    $tmp->$class->__init__($tmp, file);     // Inline this message, note that $RFile$__init__ is *not* CPS'ed
    return $tmp;
}
struct $RFile$class $RFile$methods;
$NoneType $WFile$__init__ ($WFile __self__, int descr) {
    __self__->descriptor = descr;
    return $None;
}
$R $WFile$write$local ($WFile __self__, $str s, $Cont c$cont) {
    memcpy(fd_data[__self__->descriptor].buffer,s->str,s->nbytes+1);
    int chunk_size = s->nbytes > BUF_SIZE ? BUF_SIZE : s->nbytes; 
    int r = write(__self__->descriptor,fd_data[__self__->descriptor].buffer,chunk_size);
    //  for now, assume str->nbytes < BUF_SIZE
    return $R_CONT(c$cont, $None);
}
$R $WFile$close$local ($WFile __self__, $Cont c$cont) {
    close(__self__->descriptor); 
    $init_FileDescriptorData(__self__->descriptor);
    return $R_CONT(c$cont, $None);
}
$Msg $WFile$write ($WFile __self__, $str s) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$13lambda$new(__self__, s)));
}
$Msg $WFile$close ($WFile __self__) {
    return $ASYNC((($Actor)__self__), (($Cont)env$$l$14lambda$new(__self__)));
}
void $WFile$__serialize__ ($WFile self, $Serial$state state) {
    $Actor$methods.__serialize__(($Actor)self, state);
}
$WFile $WFile$__deserialize__ ($WFile self, $Serial$state state) {
    if (!self) {
        if (!state) {
            self = malloc(sizeof(struct $WFile));
            self->$class = &$WFile$methods;
            return self;
        }
        self = $DNEW($WFile, state);
    }
    $Actor$methods.__deserialize__(($Actor)self, state);
    return self;
}
$WFile $WFile$newact(int descr) {
    $WFile $tmp = $NEWACTOR($WFile);
    $tmp->$class->__init__($tmp, descr);     // Inline this message, note that $WFile$__init__ is *not* CPS'ed
    return $tmp;
}
struct $WFile$class $WFile$methods;
int env$$done$ = 0;
void env$$__init__ () {
    if (env$$done$) return;
    env$$done$ = 1;
    {
        env$$l$1lambda$methods.$GCINFO = "env$$l$1lambda";
        env$$l$1lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$1lambda$methods.__bool__ = ($bool (*) (env$$l$1lambda))$value$methods.__bool__;
        env$$l$1lambda$methods.__str__ = ($str (*) (env$$l$1lambda))$value$methods.__str__;
        env$$l$1lambda$methods.__init__ = env$$l$1lambda$__init__;
        env$$l$1lambda$methods.__call__ = env$$l$1lambda$__call__;
        env$$l$1lambda$methods.__serialize__ = env$$l$1lambda$__serialize__;
        env$$l$1lambda$methods.__deserialize__ = env$$l$1lambda$__deserialize__;
        $register(&env$$l$1lambda$methods);
    }
    {
        env$$l$2lambda$methods.$GCINFO = "env$$l$2lambda";
        env$$l$2lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$2lambda$methods.__bool__ = ($bool (*) (env$$l$2lambda))$value$methods.__bool__;
        env$$l$2lambda$methods.__str__ = ($str (*) (env$$l$2lambda))$value$methods.__str__;
        env$$l$2lambda$methods.__init__ = env$$l$2lambda$__init__;
        env$$l$2lambda$methods.__call__ = env$$l$2lambda$__call__;
        env$$l$2lambda$methods.__serialize__ = env$$l$2lambda$__serialize__;
        env$$l$2lambda$methods.__deserialize__ = env$$l$2lambda$__deserialize__;
        $register(&env$$l$2lambda$methods);
    }
    {
        env$$l$3lambda$methods.$GCINFO = "env$$l$3lambda";
        env$$l$3lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$3lambda$methods.__bool__ = ($bool (*) (env$$l$3lambda))$value$methods.__bool__;
        env$$l$3lambda$methods.__str__ = ($str (*) (env$$l$3lambda))$value$methods.__str__;
        env$$l$3lambda$methods.__init__ = env$$l$3lambda$__init__;
        env$$l$3lambda$methods.__call__ = env$$l$3lambda$__call__;
        env$$l$3lambda$methods.__serialize__ = env$$l$3lambda$__serialize__;
        env$$l$3lambda$methods.__deserialize__ = env$$l$3lambda$__deserialize__;
        $register(&env$$l$3lambda$methods);
    }
    {
        env$$l$4lambda$methods.$GCINFO = "env$$l$4lambda";
        env$$l$4lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$4lambda$methods.__bool__ = ($bool (*) (env$$l$4lambda))$value$methods.__bool__;
        env$$l$4lambda$methods.__str__ = ($str (*) (env$$l$4lambda))$value$methods.__str__;
        env$$l$4lambda$methods.__init__ = env$$l$4lambda$__init__;
        env$$l$4lambda$methods.__call__ = env$$l$4lambda$__call__;
        env$$l$4lambda$methods.__serialize__ = env$$l$4lambda$__serialize__;
        env$$l$4lambda$methods.__deserialize__ = env$$l$4lambda$__deserialize__;
        $register(&env$$l$4lambda$methods);
    }
    {
        env$$l$5lambda$methods.$GCINFO = "env$$l$5lambda";
        env$$l$5lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$5lambda$methods.__bool__ = ($bool (*) (env$$l$5lambda))$value$methods.__bool__;
        env$$l$5lambda$methods.__str__ = ($str (*) (env$$l$5lambda))$value$methods.__str__;
        env$$l$5lambda$methods.__init__ = env$$l$5lambda$__init__;
        env$$l$5lambda$methods.__call__ = env$$l$5lambda$__call__;
        env$$l$5lambda$methods.__serialize__ = env$$l$5lambda$__serialize__;
        env$$l$5lambda$methods.__deserialize__ = env$$l$5lambda$__deserialize__;
        $register(&env$$l$5lambda$methods);
    }
    {
        env$$l$6lambda$methods.$GCINFO = "env$$l$6lambda";
        env$$l$6lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$6lambda$methods.__bool__ = ($bool (*) (env$$l$6lambda))$value$methods.__bool__;
        env$$l$6lambda$methods.__str__ = ($str (*) (env$$l$6lambda))$value$methods.__str__;
        env$$l$6lambda$methods.__init__ = env$$l$6lambda$__init__;
        env$$l$6lambda$methods.__call__ = env$$l$6lambda$__call__;
        env$$l$6lambda$methods.__serialize__ = env$$l$6lambda$__serialize__;
        env$$l$6lambda$methods.__deserialize__ = env$$l$6lambda$__deserialize__;
        $register(&env$$l$6lambda$methods);
    }
    {
        env$$l$7lambda$methods.$GCINFO = "env$$l$7lambda";
        env$$l$7lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$7lambda$methods.__bool__ = ($bool (*) (env$$l$7lambda))$value$methods.__bool__;
        env$$l$7lambda$methods.__str__ = ($str (*) (env$$l$7lambda))$value$methods.__str__;
        env$$l$7lambda$methods.__init__ = env$$l$7lambda$__init__;
        env$$l$7lambda$methods.__call__ = env$$l$7lambda$__call__;
        env$$l$7lambda$methods.__serialize__ = env$$l$7lambda$__serialize__;
        env$$l$7lambda$methods.__deserialize__ = env$$l$7lambda$__deserialize__;
        $register(&env$$l$7lambda$methods);
    }
    {
        env$$l$8lambda$methods.$GCINFO = "env$$l$8lambda";
        env$$l$8lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$8lambda$methods.__bool__ = ($bool (*) (env$$l$8lambda))$value$methods.__bool__;
        env$$l$8lambda$methods.__str__ = ($str (*) (env$$l$8lambda))$value$methods.__str__;
        env$$l$8lambda$methods.__init__ = env$$l$8lambda$__init__;
        env$$l$8lambda$methods.__call__ = env$$l$8lambda$__call__;
        env$$l$8lambda$methods.__serialize__ = env$$l$8lambda$__serialize__;
        env$$l$8lambda$methods.__deserialize__ = env$$l$8lambda$__deserialize__;
        $register(&env$$l$8lambda$methods);
    }
    {
        env$$l$9lambda$methods.$GCINFO = "env$$l$9lambda";
        env$$l$9lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$9lambda$methods.__bool__ = ($bool (*) (env$$l$9lambda))$value$methods.__bool__;
        env$$l$9lambda$methods.__str__ = ($str (*) (env$$l$9lambda))$value$methods.__str__;
        env$$l$9lambda$methods.__init__ = env$$l$9lambda$__init__;
        env$$l$9lambda$methods.__call__ = env$$l$9lambda$__call__;
        env$$l$9lambda$methods.__serialize__ = env$$l$9lambda$__serialize__;
        env$$l$9lambda$methods.__deserialize__ = env$$l$9lambda$__deserialize__;
        $register(&env$$l$9lambda$methods);
    }
    {
        env$$l$10lambda$methods.$GCINFO = "env$$l$10lambda";
        env$$l$10lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$10lambda$methods.__bool__ = ($bool (*) (env$$l$10lambda))$value$methods.__bool__;
        env$$l$10lambda$methods.__str__ = ($str (*) (env$$l$10lambda))$value$methods.__str__;
        env$$l$10lambda$methods.__init__ = env$$l$10lambda$__init__;
        env$$l$10lambda$methods.__call__ = env$$l$10lambda$__call__;
        env$$l$10lambda$methods.__serialize__ = env$$l$10lambda$__serialize__;
        env$$l$10lambda$methods.__deserialize__ = env$$l$10lambda$__deserialize__;
        $register(&env$$l$10lambda$methods);
    }
    {
        env$$l$11lambda$methods.$GCINFO = "env$$l$11lambda";
        env$$l$11lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$11lambda$methods.__bool__ = ($bool (*) (env$$l$11lambda))$value$methods.__bool__;
        env$$l$11lambda$methods.__str__ = ($str (*) (env$$l$11lambda))$value$methods.__str__;
        env$$l$11lambda$methods.__init__ = env$$l$11lambda$__init__;
        env$$l$11lambda$methods.__call__ = env$$l$11lambda$__call__;
        env$$l$11lambda$methods.__serialize__ = env$$l$11lambda$__serialize__;
        env$$l$11lambda$methods.__deserialize__ = env$$l$11lambda$__deserialize__;
        $register(&env$$l$11lambda$methods);
    }
    {
        env$$l$12lambda$methods.$GCINFO = "env$$l$12lambda";
        env$$l$12lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$12lambda$methods.__bool__ = ($bool (*) (env$$l$12lambda))$value$methods.__bool__;
        env$$l$12lambda$methods.__str__ = ($str (*) (env$$l$12lambda))$value$methods.__str__;
        env$$l$12lambda$methods.__init__ = env$$l$12lambda$__init__;
        env$$l$12lambda$methods.__call__ = env$$l$12lambda$__call__;
        env$$l$12lambda$methods.__serialize__ = env$$l$12lambda$__serialize__;
        env$$l$12lambda$methods.__deserialize__ = env$$l$12lambda$__deserialize__;
        $register(&env$$l$12lambda$methods);
    }
    {
        env$$l$13lambda$methods.$GCINFO = "env$$l$13lambda";
        env$$l$13lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$13lambda$methods.__bool__ = ($bool (*) (env$$l$13lambda))$value$methods.__bool__;
        env$$l$13lambda$methods.__str__ = ($str (*) (env$$l$13lambda))$value$methods.__str__;
        env$$l$13lambda$methods.__init__ = env$$l$13lambda$__init__;
        env$$l$13lambda$methods.__call__ = env$$l$13lambda$__call__;
        env$$l$13lambda$methods.__serialize__ = env$$l$13lambda$__serialize__;
        env$$l$13lambda$methods.__deserialize__ = env$$l$13lambda$__deserialize__;
        $register(&env$$l$13lambda$methods);
    }
    {
        env$$l$14lambda$methods.$GCINFO = "env$$l$14lambda";
        env$$l$14lambda$methods.$superclass = ($Super$class)&$Cont$methods;
        env$$l$14lambda$methods.__bool__ = ($bool (*) (env$$l$14lambda))$value$methods.__bool__;
        env$$l$14lambda$methods.__str__ = ($str (*) (env$$l$14lambda))$value$methods.__str__;
        env$$l$14lambda$methods.__init__ = env$$l$14lambda$__init__;
        env$$l$14lambda$methods.__call__ = env$$l$14lambda$__call__;
        env$$l$14lambda$methods.__serialize__ = env$$l$14lambda$__serialize__;
        env$$l$14lambda$methods.__deserialize__ = env$$l$14lambda$__deserialize__;
        $register(&env$$l$14lambda$methods);
    }
    {
        $Env$methods.$GCINFO = "$Env";
        $Env$methods.$superclass = ($Super$class)&$Actor$methods;
        $Env$methods.__serialize__ = $Env$__serialize__;
        $Env$methods.__deserialize__ = $Env$__deserialize__;
        $Env$methods.__bool__ = ($bool (*) ($Env))$Actor$methods.__bool__;
        $Env$methods.__str__ = ($str (*) ($Env))$Actor$methods.__str__;
        $Env$methods.__init__ = $Env$__init__;
        $Env$methods.stdout_write$local = $Env$stdout_write$local;
        $Env$methods.stdin_install$local = $Env$stdin_install$local;
        $Env$methods.connect$local = $Env$connect$local;
        $Env$methods.listen$local = $Env$listen$local;
        $Env$methods.exit$local = $Env$exit$local;
        $Env$methods.openR$local = $Env$openR$local;
        $Env$methods.openW$local = $Env$openW$local;
        $Env$methods.stdout_write = $Env$stdout_write;
        $Env$methods.stdin_install = $Env$stdin_install;
        $Env$methods.connect = $Env$connect;
        $Env$methods.listen = $Env$listen;
        $Env$methods.exit = $Env$exit;
        $Env$methods.openR = $Env$openR;
        $Env$methods.openW = $Env$openW;
        $Env$methods.__serialize__ = $Env$__serialize__;
        $Env$methods.__deserialize__ = $Env$__deserialize__;
        $register(&$Env$methods);
    }
    {
        $Connection$methods.$GCINFO = "$Connection";
        $Connection$methods.$superclass = ($Super$class)&$Actor$methods;
        $Connection$methods.__serialize__ = $Connection$__serialize__;
        $Connection$methods.__deserialize__ = $Connection$__deserialize__;
        $Connection$methods.__bool__ = ($bool (*) ($Connection))$Actor$methods.__bool__;
        $Connection$methods.__str__ = ($str (*) ($Connection))$Actor$methods.__str__;
        $Connection$methods.__init__ = $Connection$__init__;
        $Connection$methods.write$local = $Connection$write$local;
        $Connection$methods.close$local = $Connection$close$local;
        $Connection$methods.on_receipt$local = $Connection$on_receipt$local;
        $Connection$methods.write = $Connection$write;
        $Connection$methods.close = $Connection$close;
        $Connection$methods.on_receipt = $Connection$on_receipt;
        $Connection$methods.__serialize__ = $Connection$__serialize__;
        $Connection$methods.__deserialize__ = $Connection$__deserialize__;
        $register(&$Connection$methods);
    }
    {
        $RFile$methods.$GCINFO = "$RFile";
        $RFile$methods.$superclass = ($Super$class)&$Actor$methods;
        $RFile$methods.__bool__ = ($bool (*) ($RFile))$Actor$methods.__bool__;
        $RFile$methods.__str__ = ($str (*) ($RFile))$Actor$methods.__str__;
        $RFile$methods.__init__ = $RFile$__init__;
        $RFile$methods.readln$local = $RFile$readln$local;
        $RFile$methods.close$local = $RFile$close$local;
        $RFile$methods.readln = $RFile$readln;
        $RFile$methods.close = $RFile$close;
        $RFile$methods.__serialize__ = $RFile$__serialize__;
        $RFile$methods.__deserialize__ = $RFile$__deserialize__;
        $register(&$RFile$methods);
    }
    {
        $WFile$methods.$GCINFO = "$WFile";
        $WFile$methods.$superclass = ($Super$class)&$Actor$methods;
        $WFile$methods.__bool__ = ($bool (*) ($WFile))$Actor$methods.__bool__;
        $WFile$methods.__str__ = ($str (*) ($WFile))$Actor$methods.__str__;
        $WFile$methods.__init__ = $WFile$__init__;
        $WFile$methods.write$local = $WFile$write$local;
        $WFile$methods.close$local = $WFile$close$local;
        $WFile$methods.write = $WFile$write;
        $WFile$methods.close = $WFile$close;
        $WFile$methods.__serialize__ = $WFile$__serialize__;
        $WFile$methods.__deserialize__ = $WFile$__deserialize__;
        $register(&$WFile$methods);
    }
    int r = pipe(wakeup_pipe);
    EVENT_init();
}

void reset_timeout() {
    int r = write(wakeup_pipe[1], "!", 1);      // Write dummy data that wakes up the eventloop thread
}

void *$eventloop(void *arg) {
#if defined(IS_MACOS)
    pthread_setname_np("IO");
#else
    pthread_setname_np(pthread_self(), "IO");
#endif

    pthread_setspecific(self_key, NULL);
    while(1) {
        EVENT_type kev;                                                          // struct epoll_event epev;

        struct sockaddr_in addr;
        socklen_t socklen = sizeof(addr);
        int fd2;
        int count;
        struct timespec tspec, *timeout;

        handle_timeout();
        time_t next_time = next_timeout();
        if (next_time) {
            time_t now = current_time();
            time_t offset = next_time - now;
            tspec.tv_sec = offset / 1000000;
            tspec.tv_nsec = 1000 * (offset % 1000000);
            //printf("## Current time is setting timer offset %ld sec, %ld nsec\n", tspec.tv_sec, tspec.tv_nsec);
            timeout = &tspec;
        } else {
            timeout = NULL;
        }

        // Blocking call
        int nready = EVENT_wait(&kev, timeout);

        if (nready<0) {
            fprintf(stderr, "EVENT error: %s\n", strerror(errno));
            continue;
        }
        if (nready == 0) {
            continue;
        }
        if (EVENT_is_error(&kev)) {
            fprintf(stderr, "EVENT error: %s\n", strerror(EVENT_errno(&kev)));
            continue;
        }
        if (EVENT_is_wakeup(&kev)) {
            char dummy;
            int r = read(wakeup_pipe[0], &dummy, 1);      // Consume dummy data, reset timer at the start of next turn
            continue;
        }
        int fd = EVENT_fd(&kev);
        if (EVENT_is_eof(&kev)) {
            $str msg = $Times$str$witness->$class->__add__($Times$str$witness,$getName(fd),to$str(" closed connection\n"));
            if (fd_data[fd].errhandler)
                fd_data[fd].errhandler->$class ->__call__(fd_data[fd].errhandler,msg);
            else {
                perror("Remote host closed connection");
                exit(-1);
            }
            EVENT_del_read(fd);
        }
        switch (fd_data[fd].kind) {
            case connecthandler:
                if (EVENT_is_read(&kev)) {              // we are a listener and someone tries to connect
                    while ((fd2 = accept(fd, (struct sockaddr *)&fd_data[fd].sock_addr,&socklen)) != -1) {
                      fcntl(fd2,F_SETFL,O_NONBLOCK);
                      fd_data[fd2].kind = connecthandler;
                      fd_data[fd2].chandler = fd_data[fd].chandler;
                      fd_data[fd2].sock_addr = fd_data[fd].sock_addr;
                      bzero(fd_data[fd2].buffer,BUF_SIZE);
                      EVENT_add_read(fd2);
                      EVENT_mod_read_once(fd);
                      setupConnection(fd2);
                      printf("%s %s\n","Connection from",$getName(fd2)->str);
                    }
                } else { // we are a client and a delayed connection attempt has succeeded
                    setupConnection(fd);
                }
                break;
            case readhandler:  // data has arrived on fd to fd_data[fd].buffer
                if (EVENT_fd_is_read(fd)) {
                    count = read(fd,&fd_data[fd].buffer,BUF_SIZE);
                    if (count < BUF_SIZE)
                        fd_data[fd].buffer[count] = 0;
                    fd_data[fd].rhandler->$class->__call__(fd_data[fd].rhandler,to$str(fd_data[fd].buffer));
                } else {
                    fprintf(stderr,"internal error: readhandler/event filter mismatch on descriptor %d\n",fd);
                    exit(-1);
                }
                break;
            case nohandler:
                fprintf(stderr,"internal error: no event handler on descriptor %d\n",fd);
                exit(-1);
        }
        pthread_mutex_lock(&sleep_lock);
        pthread_cond_signal(&work_to_do);
        pthread_mutex_unlock(&sleep_lock);

    }
    return NULL;
}
