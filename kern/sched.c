#include <env.h>
#include <pmap.h>
#include <printk.h>

/* Overview:
 *   Implement a round-robin scheduling to select a runnable env and schedule it using 'env_run'.
 *
 * Post-Condition:
 *   If 'yield' is set (non-zero), 'curenv' should not be scheduled again unless it is the only
 *   runnable env.
 *
 * Hints:
 *   1. The variable 'count' used for counting slices should be defined as 'static'.
 *   2. Use variable 'env_sched_list', which contains and only contains all runnable envs.
 *   3. You shouldn't use any 'return' statement because this function is 'noreturn'.
 */
/*
void schedule(int yield) {
	static int count = 0; // remaining time slices of current env
	struct Env *e = curenv;

	if (yield || count == 0 || e == NULL || e->env_status != ENV_RUNNABLE) {
		if (e != NULL) {
			TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
			if (e->env_status == ENV_RUNNABLE) {
				TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
			}
		}
		e = TAILQ_FIRST(&env_sched_list);
		if (e == NULL) {
			panic("schedule: no runnable envs are available !\n");
		}
		count = e->env_pri;
	}
	count--;
	env_run(e);
}
*/
void schedule(int yield) {
	static int clock = -1; // 当前时间片，从 0 开始计数
	clock++;

	/* (1) 遍历 env_edf_sched_list，如果进程进入了新的运行周期（可通过 clock == env_period_deadline 判断），则更新 env_period_deadline，并将 env_runtime_left 设置为 env_edf_runtime。 */
	struct Env *env; // 循环变量

	LIST_FOREACH (env, &env_edf_sched_list, env_edf_sched_link) {
		if(clock == env->env_period_deadline) {
			env->env_period_deadline += env->env_edf_period;
			env->env_runtime_left = env->env_edf_runtime;
		}
	}

	u_int min_deadline = -1, min_id = -1;
	struct Env *min_env;
	/* (2) 遍历 env_edf_sched_list，选取 env_runtime_left 大于 0 且 env_period_deadline 最小的进程调度（若相同，则选择 env_id 最小的进程）。如果不存在这样的进程，则不进行调度。 */
	LIST_FOREACH (env, &env_edf_sched_list, env_edf_sched_link) {
		if(env->env_runtime_left > 0) {
			if(min_deadline == -1 || env->env_period_deadline < min_deadline || (env->env_period_deadline == min_deadline && env->env_id < min_id)) {
				min_deadline = env->env_period_deadline;
				min_id = env->env_id;
				min_env = env;
			}
		}
	}
	if(min_deadline != -1) {
		printk("edf sched runtimeLeft: %d\n", min_env->env_runtime_left);
		min_env->env_runtime_left--;
		env_run(min_env);
	}

	/* (3) 使用课下实现的 RR 算法调度 env_sched_list 中的进程。 */
	static int count = 0; // remaining time slices of current env
	static struct Env *last_RR = NULL;
	struct Env *e = last_RR; // 请根据提示修改这行代码

	if (yield || count == 0 || e == NULL || e->env_status != ENV_RUNNABLE) {
		if (e != NULL) {
			TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
			if (e->env_status == ENV_RUNNABLE) {
				TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
			}
		}
		e = TAILQ_FIRST(&env_sched_list);
		last_RR = e;
		if (e == NULL) {
			panic("schedule: no runnable envs are available !\n");
		}
		count = e->env_pri;
	}
	count--;
	env_run(e);
}
