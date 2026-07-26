const { app } = require('@azure/functions');
const { checkRateLimit } = require('./rateLimiter');

app.http('geocodeProxy', {
    methods: ['GET'],
    authLevel: 'anonymous',
    route: 'geocode',
    handler: async (request, context) => {
        const appKey = request.headers.get('x-compendia-key');
        if (!appKey || appKey !== process.env.COMPENDIA_APP_KEY) {
            return { status: 401, body: 'Unauthorized' };
        }

        const rateResult = await checkRateLimit(request, context, 'geocode');
        if (!rateResult.allowed) {
            return { status: 429, body: 'Rate limit exceeded' };
        }

        const lat = request.query.get('lat');
        const lon = request.query.get('lon');

        if (!lat || !lon) {
            return { status: 400, body: 'Missing required query parameters: lat, lon' };
        }

        const mapboxToken = process.env.MAPBOX_TOKEN;

        if (!mapboxToken) {
            context.error('MAPBOX_TOKEN is not configured');
            return { status: 500, body: 'Server configuration error' };
        }

        const upstream =
            `https://api.mapbox.com/geocoding/v5/mapbox.places/${lon},${lat}.json` +
            `?types=place,locality,district&access_token=${mapboxToken}`;

        let response;
        try {
            response = await fetch(upstream);
        } catch (err) {
            context.error('Upstream geocode fetch failed', err);
            return { status: 502, body: 'Bad gateway' };
        }

        if (!response.ok) {
            return { status: response.status, body: `Upstream error: ${response.statusText}` };
        }

        // Return the raw Mapbox FeatureCollection JSON unchanged so the desktop
        // client's existing parsing code works without modification.
        const json = await response.json();

        return {
            status: 200,
            jsonBody: json,
            headers: {
                'Cache-Control': 'public, max-age=3600',
            },
        };
    },
});
