/// @file Context.cpp
/// Caliper Context class implementation

#include "Context.h"
#include "SigsafeRWLock.h"

#include <algorithm>
#include <utility>
#include <vector>


using namespace cali;
using namespace std;

struct Context::ContextImpl 
{
    // --- data

    typedef vector< pair<ctx_id_t, uint64_t> > env_vec_t;

    vector<env_vec_t>     m_environments;
    mutable SigsafeRWLock m_lock;


    // --- constructor

    ContextImpl() {
        env_vec_t env;
        env.reserve(8); // FIXME: make configurable
        m_environments.push_back(move(env));
    }

    // --- interface

    ctx_id_t clone_environment(ctx_id_t env) {
        if (env == CTX_INV_ID || env >= m_environments.size())
            return CTX_INV_ID;

        m_environments.emplace_back( m_environments[env] );

        return m_environments.size();
    }

    void release_environment(ctx_id_t env) {
        m_environments.erase(m_environments.begin() + env);
    }

    size_t record_size(ctx_id_t env) const {
        return env < m_environments.size() ? m_environments[env].size() * 2 : 0;
    }

    size_t get_context(ctx_id_t env, uint64_t buf[], size_t len) const {
        if (!(env < m_environments.size()))
            return 0;

        auto env_p = m_environments.begin() + env;
        size_t s = 0;

        for (auto it = env_p->begin() ; (s + 1) < len && it != env_p->end(); ++it, s += 2) {
            buf[s]   = it->first;
            buf[s+1] = it->second;
        }

        return s;
    }

    pair<bool, uint64_t> get(ctx_id_t env, ctx_id_t key) const {
        if (!(env < m_environments.size()))
            return make_pair<bool, uint64_t>(false, 0);

        auto env_p = m_environments.begin() + env;
        auto it = lower_bound(env_p->begin(), env_p->end(), make_pair(key, uint64_t(0)));

        if (it == env_p->end() || it->first != key)
            return make_pair<bool, uint64_t>(false, 0);

        return make_pair(true, it->second);
    }

    ctx_err set (ctx_id_t env, ctx_id_t key, uint64_t value, bool /* global */) {
        if (!(env < m_environments.size()))
            return CTX_EINV;

        auto env_p = m_environments.begin() + env;
        auto it = lower_bound(env_p->begin(), env_p->end(), make_pair(key, uint64_t(0)));

        if (it != env_p->end() && it->first == key)
            it->second = value;
        else
            env_p->insert(it, make_pair(key, value));

        return CTX_SUCCESS;
    }

    ctx_err unset(ctx_id_t env, uint64_t key) {
        if (!(env < m_environments.size()))
            return CTX_EINV;

        auto env_p = m_environments.begin() + env;
        auto it = lower_bound(env_p->begin(), env_p->end(), make_pair(key, uint64_t(0)));

        if (it != env_p->end() && it->first == key)
            env_p->erase(it);

        return CTX_SUCCESS;
    }
};

Context::Context()
    : mP(new ContextImpl)
{ 
}

Context::~Context()
{
    mP.reset();
}

ctx_id_t Context::clone_environment(ctx_id_t env)
{
    mP->m_lock.wlock();
    ctx_id_t ret = mP->clone_environment(env);
    mP->m_lock.unlock();

    return ret;
}

void Context::release_environment(ctx_id_t env)
{
    mP->m_lock.wlock();
    mP->release_environment(env);
    mP->m_lock.unlock();
}

size_t Context::context_size(ctx_id_t env) const 
{
    mP->m_lock.rlock();
    size_t ret = mP->record_size(env);
    mP->m_lock.unlock();

    return ret;
}

size_t Context::get_context(ctx_id_t env, uint64_t buf[], size_t len) const
{
    mP->m_lock.rlock();
    size_t ret = mP->get_context(env, buf, len);
    mP->m_lock.unlock();

    return ret;
}

pair<bool, uint64_t> Context::get(ctx_id_t env, ctx_id_t key) const
{
    mP->m_lock.rlock();
    auto p= mP->get(env, key);
    mP->m_lock.unlock();

    return p;
}

ctx_err Context::set(ctx_id_t env, ctx_id_t key, uint64_t value, bool global)
{
    mP->m_lock.wlock();
    ctx_err ret = mP->set(env, key, value, global);
    mP->m_lock.unlock();

    return ret;
}

ctx_err Context::unset(ctx_id_t env, ctx_id_t key)
{
    mP->m_lock.wlock();
    ctx_err ret = mP->unset(env, key);
    mP->m_lock.unlock();

    return ret;
}