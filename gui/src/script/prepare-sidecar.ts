import fs from 'fs-extra';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const projectRoot = path.resolve(__dirname, '..', '..', '..');

const cppBuildDir = path.join(projectRoot, 'core', 'build');

const sidecarDestDir = path.join(projectRoot, 'gui', 'src-tauri', 'core');

const mode = process.argv[2];
if (!mode) {
    console.error('[Sidecar 准备] 严重错误：缺少构建模式参数。');
    console.error('[Sidecar 准备] 预期命令格式：`tsx prepare-sidecar.ts [debug|release]`');
    process.exit(1);
}
if (mode !== 'debug' && mode !== 'release') {
    console.error(`[Sidecar 准备] 严重错误：构建模式参数无效 "${mode}"。`);
    console.error('[Sidecar 准备] 必须是 "debug" 或 "release"。');
    process.exit(1);
}


const sourceDir = path.join(cppBuildDir, mode, 'bin', 'core');

console.log(`[Sidecar 准备] 开始准备 ${mode} 模式。`);
console.log(`[Sidecar 准备] 项目根目录：${projectRoot}`);
console.log(`[Sidecar 准备] 源路径：${sourceDir}`);
console.log(`[Sidecar 准备] 目标路径：${sidecarDestDir}`);

try {
    fs.emptyDirSync(sidecarDestDir);
    console.log('[Sidecar 准备] 已清空目标目录。');

    if (!fs.existsSync(sourceDir)) {
        console.error(`\n[Sidecar 准备] 严重错误：未找到源目录！`);
        console.error(`[Sidecar 准备] > 目录 "${sourceDir}" 不存在。`);
        console.error(`[Sidecar 准备] > 请先确保已在 'core' 目录成功编译 C++ 项目。\n`);
        process.exit(1);
    }

    fs.copySync(sourceDir, sidecarDestDir);
    console.log(`[Sidecar 准备] 已成功复制产物：${path.basename(sourceDir)}。`);

} catch (error) {
    console.error('[Sidecar 准备] 文件操作发生未知错误：', error);
    process.exit(1);
}

console.log('[Sidecar 准备] 准备完成，Tauri 可继续。');
