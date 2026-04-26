const { app } = require('@azure/functions');

app.http('tileProxy', {
    methods: ['GET'],
    authLevel: 'anonymous',
    route: 'tiles/{z}/{x}/{y}',
    handler: async (request, context) => {
        const appKey = request.headers.get('x-compendia-key');
        if (!appKey || appKey !== process.env.COMPENDIA_APP_KEY) {
            return { status: 401, body: 'Unauthorized' };
        }

        const { z, x, y } = request.params;
        const mapboxToken = process.env.MAPBOX_TOKEN;

        if (!mapboxToken) {
            context.error('MAPBOX_TOKEN is not configured');
            return { status: 500, body: 'Server configuration error' };
        }

        const upstream =
            `https://api.mapbox.com/styles/v1/mapbox/streets-v11/tiles/256/${z}/${x}/${y}` +
            `?access_token=${mapboxToken}`;

        let response;
        try {
            response = await fetch(upstream);
        } catch (err) {
            context.error('Upstream tile fetch failed', err);
            return { status: 502, body: 'Bad gateway' };
        }

        if (!response.ok) {
            return { status: response.status, body: `Upstream error: ${response.statusText}` };
        }

        const imageBuffer = Buffer.from(await response.arrayBuffer());

        return {
            status: 200,
            body: imageBuffer,
            headers: {
                'Content-Type': response.headers.get('content-type') || 'image/png',
                'Cache-Control': 'public, max-age=86400',
            },
        };
    },
});
