/* ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''' *\
 *        .NN.        _____ _____ _____  _    _                 This file is part of CGRU
 *        hMMh       / ____/ ____|  __ \| |  | |       - The Free And Open Source CG Tools Pack.
 *       sMMMMs     | |   | |  __| |__) | |  | |  CGRU is licensed under the terms of LGPLv3, see files
 * <yMMMMMMMMMMMMMMy> |   | | |_ |  _  /| |  | |    COPYING and COPYING.lesser inside of this folder.
 *   `+mMMMMMMMMNo` | |___| |__| | | \ \| |__| |          Project-Homepage: http://cgru.info
 *     :MMMMMMMM:    \_____\_____|_|  \_\\____/        Sourcecode: https://github.com/CGRU/cgru
 *     dMMMdmMMMd     A   F   A   N   A   S   Y
 *    -Mmo.  -omM:                                           Copyright © by The CGRU team
 *    '          '
\* ....................................................................................................... */

/*
	poolsrv.h - Server side renders pool class.
*/
#pragma once

#include "../libafanasy/pool.h"

#include "afnodesrv.h"

class Action;
class PoolsContainer;
class RenderAf;
class UserAf;

/// Server side of Afanasy pool.
class PoolSrv : public af::Pool, public AfNodeSrv
{
public:
	/// Create a new pool on a new render.
	PoolSrv(PoolSrv * i_parent, const std::string & i_path);

	/// Construct from store.
	PoolSrv(const std::string & i_store_dir);

	~PoolSrv();

	bool setParent(PoolSrv * i_parent);

	bool initialize();

	bool hasPool(const PoolSrv * i_pool) const;
	void addPool(PoolSrv * i_pool);
	void removePool(PoolSrv * i_pool);

	bool hasRender(const RenderAf * i_render) const;
	void addRender(RenderAf * i_render, UserAf * i_user);
	void removeRender(RenderAf * i_render, UserAf * i_user);

	void v_refresh(time_t i_current_time, AfContainer * i_container, MonitorContainer * i_monitoring);

	virtual void v_action(Action & i_action);

	void logAction(const Action & i_action, const std::string & i_node_name);

	virtual int v_calcWeight() const;

public:
	inline static void setPoolsContainer(PoolsContainer * i_pools ) { ms_pools = i_pools;}

private:
	void deletePool(Action & o_action, MonitorContainer * i_monitoring);

private:
	PoolSrv * m_parent;
	std::list<PoolSrv*> m_pools_list;
	std::list<RenderAf*> m_renders_list;

private:
	static PoolsContainer * ms_pools;
};