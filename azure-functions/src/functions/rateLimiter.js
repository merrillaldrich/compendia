const LIMITS = { tile: 500, geocode: 100 };

let tableClientPromise = null;

function getTableClient() {
    if (tableClientPromise) return tableClientPromise;
    tableClientPromise = (async () => {
        const { TableClient } = require('@azure/data-tables');
        const client = TableClient.fromConnectionString(
            process.env.AzureWebJobsStorage, 'compratelimits');
        await client.createTable();
        return client;
    })();
    return tableClientPromise;
}

function getClientIp(request) {
    const xff = request.headers.get('x-forwarded-for') || '';
    return xff.split(',')[0].trim().replace(/:\d+$/, '') || 'unknown';
}

async function checkRateLimit(request, context, limitType) {
    if (process.env.AzureWebJobsStorage === 'UseDevelopmentStorage=true') {
        return { allowed: true };
    }

    const ip = getClientIp(request);
    const dateKey = new Date().toISOString().slice(0, 10);
    const countField = limitType === 'tile' ? 'tileCount' : 'geocodeCount';
    const limit = LIMITS[limitType];
    const client = await getTableClient();

    async function attempt() {
        let entity, etag;
        try {
            const result = await client.getEntity(dateKey, ip);
            entity = { ...result };
            etag = result.etag;
        } catch (err) {
            if (err.statusCode !== 404) throw err;
            entity = { partitionKey: dateKey, rowKey: ip, tileCount: 0, geocodeCount: 0 };
            etag = undefined;
        }

        const newCount = (entity[countField] || 0) + 1;
        if (newCount > limit) return { allowed: false };

        entity[countField] = newCount;
        if (etag === undefined) {
            await client.createEntity(entity);
        } else {
            await client.updateEntity(entity, 'Replace', { etag });
        }
        return { allowed: true };
    }

    try {
        return await attempt();
    } catch {
        try {
            return await attempt();
        } catch (err) {
            context.warn('Rate limit storage error, failing open', err.message);
            return { allowed: true };
        }
    }
}

module.exports = { checkRateLimit };
