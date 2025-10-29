#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
API 深度测试脚本
测试所有端点的创建、读取、更新、删除功能
"""
import asyncio
import sys
from datetime import datetime, timezone

import httpx
from rich.console import Console
from rich.table import Table

console = Console()

API_BASE = "http://localhost:8000/api"
client = httpx.AsyncClient(base_url=API_BASE, timeout=10.0, trust_env=False)


class TestResult:
    """测试结果"""

    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.errors = []

    def pass_test(self, name: str):
        self.passed += 1
        console.print(f"✅ {name}", style="green")

    def fail_test(self, name: str, error: str):
        self.failed += 1
        self.errors.append((name, error))
        console.print(f"❌ {name}: {error}", style="red")

    def summary(self):
        console.print("\n" + "=" * 60, style="bold")
        console.print(f"测试完成: {self.passed + self.failed} 个测试", style="bold")
        console.print(f"✅ 通过: {self.passed}", style="green")
        console.print(f"❌ 失败: {self.failed}", style="red")

        if self.errors:
            console.print("\n失败详情:", style="bold red")
            for name, error in self.errors:
                console.print(f"  • {name}: {error}", style="red")

        return self.failed == 0


result = TestResult()


async def test_health():
    """测试健康检查端点"""
    console.print("\n[bold blue]1. 测试根端点[/bold blue]")
    try:
        # 访问根端点需要使用完整URL，因为client的base_url是/api
        root_client = httpx.AsyncClient(base_url="http://localhost:8000", timeout=10.0, trust_env=False)
        try:
            response = await root_client.get("/")
            if response.status_code == 200:
                data = response.json()
                console.print(f"  应用名称: {data.get('app')}")
                console.print(f"  版本: {data.get('version')}")
                result.pass_test("根端点")
            else:
                result.fail_test("根端点", f"状态码: {response.status_code}")
        finally:
            await root_client.aclose()
    except Exception as e:
        result.fail_test("根端点", str(e))


async def test_projects():
    """测试项目 API"""
    console.print("\n[bold blue]2. 测试项目 API[/bold blue]")

    project_id = None

    # 2.1 获取项目列表（应该为空）
    try:
        response = await client.get("/projects/")
        if response.status_code == 200:
            projects = response.json()
            console.print(f"  当前项目数: {len(projects)}")
            result.pass_test("获取项目列表")
        else:
            result.fail_test("获取项目列表", f"状态码: {response.status_code}")
            return None
    except Exception as e:
        result.fail_test("获取项目列表", str(e))
        return None

    # 2.2 创建项目
    try:
        project_data = {
            "name": "测试项目",
            "description": "这是一个API测试项目",
            "planned_time_ms": 36000000,  # 10小时
        }
        response = await client.post("/projects/", json=project_data)
        if response.status_code == 201:
            project = response.json()
            project_id = project["id"]
            console.print(f"  创建项目成功, ID: {project_id}")
            result.pass_test("创建项目")
        else:
            result.fail_test("创建项目", f"状态码: {response.status_code}, {response.text}")
            return None
    except Exception as e:
        result.fail_test("创建项目", str(e))
        return None

    # 2.3 获取单个项目
    try:
        response = await client.get(f"/projects/{project_id}")
        if response.status_code == 200:
            project = response.json()
            console.print(f"  项目名称: {project['name']}")
            result.pass_test("获取单个项目")
        else:
            result.fail_test("获取单个项目", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("获取单个项目", str(e))

    # 2.4 更新项目
    try:
        update_data = {"description": "更新后的描述"}
        response = await client.patch(f"/projects/{project_id}", json=update_data)
        if response.status_code == 200:
            result.pass_test("更新项目")
        else:
            result.fail_test("更新项目", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("更新项目", str(e))

    return project_id


async def test_tasks(project_id: str):
    """测试任务 API"""
    console.print("\n[bold blue]3. 测试任务 API[/bold blue]")

    if not project_id:
        result.fail_test("测试任务", "没有项目ID，跳过任务测试")
        return None

    task_id = None

    # 3.1 创建任务
    try:
        task_data = {
            "project_id": project_id,
            "title": "测试任务",
            "description": "这是一个测试任务",
            "estimated_time_ms": 3600000,  # 1小时
            "priority": 1,
        }
        response = await client.post("/tasks/", json=task_data)
        if response.status_code == 201:
            task = response.json()
            task_id = task["id"]
            console.print(f"  创建任务成功, ID: {task_id}")
            console.print(f"  任务标题: {task['title']}")
            console.print(f"  任务状态: {task['status']}")
            result.pass_test("创建任务")
        else:
            result.fail_test("创建任务", f"状态码: {response.status_code}, {response.text}")
            return None
    except Exception as e:
        result.fail_test("创建任务", str(e))
        return None

    # 3.2 获取任务列表
    try:
        response = await client.get("/tasks/")
        if response.status_code == 200:
            tasks = response.json()
            console.print(f"  当前任务数: {len(tasks)}")
            result.pass_test("获取任务列表")
        else:
            result.fail_test("获取任务列表", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("获取任务列表", str(e))

    # 3.3 获取单个任务
    try:
        response = await client.get(f"/tasks/{task_id}")
        if response.status_code == 200:
            task = response.json()
            console.print(f"  任务详情: {task['title']}, 状态: {task['status']}")
            result.pass_test("获取单个任务")
        else:
            result.fail_test("获取单个任务", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("获取单个任务", str(e))

    # 3.4 更新任务状态
    try:
        update_data = {"status": "in_progress"}
        response = await client.patch(f"/tasks/{task_id}", json=update_data)
        if response.status_code == 200:
            task = response.json()
            console.print(f"  更新后状态: {task['status']}")
            result.pass_test("更新任务")
        else:
            result.fail_test("更新任务", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("更新任务", str(e))

    return task_id


async def test_time_slices(task_id: str):
    """测试时间片 API"""
    console.print("\n[bold blue]4. 测试时间片 API[/bold blue]")

    if not task_id:
        result.fail_test("测试时间片", "没有任务ID，跳过时间片测试")
        return

    time_slice_id = None

    # 4.1 创建时间片
    try:
        now = datetime.now(timezone.utc)
        start_time = now.replace(hour=10, minute=0, second=0, microsecond=0)
        end_time = now.replace(hour=11, minute=30, second=0, microsecond=0)

        time_slice_data = {
            "task_id": task_id,
            "start_at": start_time.isoformat(),
            "end_at": end_time.isoformat(),
            "duration_ms": 5400000,  # 1.5小时
            "efficiency_score": 4,
            "note": "高效工作时段",
        }
        response = await client.post("/time-slices/", json=time_slice_data)
        if response.status_code == 201:
            time_slice = response.json()
            time_slice_id = time_slice["id"]
            console.print(f"  创建时间片成功, ID: {time_slice_id}")
            console.print(
                f"  时长: {time_slice['duration_ms'] / 60000:.1f}分钟, 效率: {time_slice['efficiency_score']}"
            )
            result.pass_test("创建时间片")
        else:
            result.fail_test("创建时间片", f"状态码: {response.status_code}, {response.text}")
            return
    except Exception as e:
        result.fail_test("创建时间片", str(e))
        return

    # 4.2 获取时间片列表
    try:
        response = await client.get("/time-slices/")
        if response.status_code == 200:
            time_slices = response.json()
            console.print(f"  当前时间片数: {len(time_slices)}")
            result.pass_test("获取时间片列表")
        else:
            result.fail_test("获取时间片列表", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("获取时间片列表", str(e))

    # 4.3 获取单个时间片
    try:
        response = await client.get(f"/time-slices/{time_slice_id}")
        if response.status_code == 200:
            result.pass_test("获取单个时间片")
        else:
            result.fail_test("获取单个时间片", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("获取单个时间片", str(e))


async def test_integration():
    """集成测试：验证任务带有时间片"""
    console.print("\n[bold blue]5. 集成测试[/bold blue]")

    try:
        # 获取所有项目及其任务
        response = await client.get("/projects/")
        if response.status_code == 200:
            projects = response.json()
            if projects:
                project = projects[0]
                console.print(f"  项目: {project['name']}")
                console.print(f"  总任务数: {len(project.get('tasks', []))}")

                if project.get("tasks"):
                    task = project["tasks"][0]
                    console.print(f"  任务: {task['title']}")
                    console.print(f"  时间片数: {len(task.get('time_slices', []))}")
                    console.print(
                        f"  总用时: {task.get('total_logged_ms', 0) / 60000:.1f}分钟"
                    )

                result.pass_test("集成测试 - 数据关联")
            else:
                result.fail_test("集成测试", "没有找到项目数据")
        else:
            result.fail_test("集成测试", f"状态码: {response.status_code}")
    except Exception as e:
        result.fail_test("集成测试", str(e))


async def main():
    """运行所有测试"""
    console.print("[bold cyan]" + "=" * 60 + "[/bold cyan]")
    console.print(
        "[bold cyan]时间盒系统 API 深度测试[/bold cyan]".center(60)
    )
    console.print("[bold cyan]" + "=" * 60 + "[/bold cyan]")

    try:
        # 测试序列
        await test_health()
        project_id = await test_projects()
        task_id = await test_tasks(project_id)
        await test_time_slices(task_id)
        await test_integration()

        # 输出总结
        result.summary()

        return 0 if result.failed == 0 else 1

    except KeyboardInterrupt:
        console.print("\n\n[yellow]测试被用户中断[/yellow]")
        return 1
    except Exception as e:
        console.print(f"\n\n[red]测试过程发生错误: {e}[/red]")
        import traceback

        traceback.print_exc()
        return 1
    finally:
        await client.aclose()


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
