const WebSocket = require('ws');
const express = require('express');
const https = require('https');
const http = require('http');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const port = 3000;
let latestData = null;
let token = null;

// === 请替换为你的实际账号信息 ===
const username = "hjnb"; // IAM用户名
const password = "H123456789j@"; // IAM密码
const domain   = "hjnb1314"; // 账号所属主账号名
const project  = "cn-north-4";
const projectId = "cd78f21bdcc148a389db46e1ca4da21e"; // 控制台可查
const deviceId  = "682dab1284adf27cda5af276_GPS_DHT11_GM";    // 设备ID
const serviceId = "GPS_DHT11_GM"; // 你的服务ID

function getToken(cb) {
    const authData = JSON.stringify({
        "auth": {
            "identity": {
                "methods": ["password"],
                "password": {
                    "user": {
                        "name": username,
                        "password": password,
                        "domain": { "name": domain }
                    }
                }
            },
            "scope": { "project": { "name": project } }
        }
    });
    const options = {
        hostname: 'iam.cn-north-4.myhuaweicloud.com',
        path: '/v3/auth/tokens',
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    };

    const req = https.request(options, (res) => {
        let data = '';
        res.on('data', chunk => { data += chunk; });
        res.on('end', () => {
            token = res.headers['x-subject-token'];
            if (token) {
                console.log('[Token] 获取成功');
                if (cb) cb(token);
            } else {
                console.error('[Token] 获取失败:', data);
            }
        });
    });
    req.on('error', (e) => {
        console.error('[Token] 请求出错:', e.message);
    });
    req.write(authData);
    req.end();
}

function fetchData() {
    if (!token) {
        console.log('[Data] Token未初始化，先获取Token');
        getToken(() => fetchData());
        return;
    }
    const options = {
        hostname: 'd82876d880.st1.iotda-app.cn-north-4.myhuaweicloud.com',
        path: `/v5/iot/${projectId}/devices/${deviceId}/shadow`,
        method: 'GET',
        headers: {
            'Content-Type': 'application/json',
            'X-Auth-Token': token
        }
    };
    const req = https.request(options, (res) => {
        let data = '';
        res.on('data', chunk => { data += chunk; });
        res.on('end', () => {
            if (res.statusCode !== 200) {
                console.error(`[Data] 状态码${res.statusCode}, body: ${data}`);
                if (res.statusCode === 401 || res.statusCode === 403) {
                    // Token失效，重取
                    getToken();
                }
                return;
            }
            try {
                const responseData = JSON.parse(data);
                const shadowArr = responseData.shadow || [];
                // 查找服务ID对应的属性
                const service = shadowArr.find(s => s.service_id === serviceId);
                if (service?.reported?.properties) {
                    latestData = service.reported.properties;
                    console.log(`[Data] 最新属性:`, latestData);

                    // WebSocket推送
                    wss.clients.forEach(client => {
                        if (client.readyState === WebSocket.OPEN) {
                            client.send(JSON.stringify(latestData));
                        }
                    });
                }
            } catch (e) {
                console.error('[Data] JSON解析出错:', e);
            }
        });
    });
    req.on('error', (e) => {
        console.error('[Data] 请求出错:', e.message);
    });
    req.end();
}

// 定时获取token（23小时一次）
setInterval(() => getToken(), 23 * 60 * 60 * 1000);

// 定时拉数据
setInterval(fetchData, 5000); // 5秒轮询

// 首次获取token
getToken();

// HTTP服务
server.listen(port, () => {
    console.log(`服务器运行在端口 ${port}`);
});
