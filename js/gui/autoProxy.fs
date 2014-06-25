uniform sampler2D rgbImage;
uniform sampler2D depthImg;

varying highp vec2 texCoo;
varying highp float sampled_depth;      // Splat-centered depth
varying highp vec2 depth_e;             // For approximating the intra-splat depth, depth = sampled_depth + depth_e' * c

varying highp float frag_depth;
varying highp vec2 frag_depth_e;

varying highp mat2 intraSplatTexCooTransform;
varying highp mat2 intraSplatTexCooTransform2;

uniform highp float splats_x;
uniform highp float splats_y;
uniform highp int splatSetIndex;
uniform highp int vp_width;
uniform highp int vp_height;
uniform highp vec3 backgroundCol;

#define PI 3.1415926535

#ifdef DEBUG
varying highp float actualSplatOverlap;         // Only used for debugging purposes in the FS
uniform int debugSplatCol;
uniform int decayMode;
uniform int roundSplats;
uniform int useISTC;
uniform int splatOutline;
varying highp float splat_i, splat_j;
#endif

uniform int useBlending;
#ifdef USE_FRAG_DEPTH_EXT
const int mostRecentProxyModelOffset = 7;
#else
const int mostRecentProxyModelOffset = 1;
#endif




void main(void)
{
    // if (splatSetIndex!=-1) discard; // Only showing "the most recent proxy model"
    // if ((splatSetIndex!=-1) && (splatSetIndex!=0) ) discard;
    // if (splatSetIndex>0) discard;

    highp float src_alpha = 1.0;
    if (useBlending>0) {
	src_alpha = 0.1;
	if (splatSetIndex==-1)
	    src_alpha = 1.0;
    }
    
    highp vec2 c = gl_PointCoord-vec2(0.5);   // c in [-0.5, 0.5]^2

#ifdef DEBUG
    highp float r_squared = dot(c, c);        // r_squared in [0, 0.5], radius squared for the largest inscribed circle is 0.25
    					      // radius squared for the smallest circle containing the 'square splat' is 0.5
    if ( (roundSplats>0) && (r_squared>0.25) ) {
        discard;
    }
    // Decay factor = 1.0 in splat center, tending toward 0 at circular rim.
    // In the distance sqrt(2*(0.5/overlap)^2), i.e., r_squared=2*(0.5/overlap)^2, we get decay=0.25,
    // which corresponds to the maximum "depth-disabled accumulated splat value" of 1.0 for a regular grid of splats.
    highp float decay = exp(actualSplatOverlap*actualSplatOverlap*log(1.0/16.0)*r_squared);
#endif

    // Adjusting for intra-splat texture coordinate.
    highp vec2 tc = texCoo;
    tc = tc + vec2(0.5/float(vp_width), 0.5/float(vp_height)); // Must we add this to get sampling mid-texel?!
#ifdef DEBUG
    if (useISTC>0) {
	tc = tc + intraSplatTexCooTransform * vec2(c.x, -c.y); // Flip needed because texture is flipped, while gl_PointCoord is not?!;
    }
#else
    tc = tc + intraSplatTexCooTransform * vec2(c.x, -c.y); // Flip needed because texture is flipped, while gl_PointCoord is not?!;
#endif

    // Discarding fragments that would look up depth and color outside the rendered scene Note that this will remove
    // parts of primitives containing "background pixels" from the rgb texture, it will not cause these to be replaced
    // by other parts of the geometry, these are simply not available in the proxy model.  Note also that this will not
    // remove parts of primitives that are rotated outside of the correct geometry.  That is impossible to do, we do not
    // have the necessary information.
    highp float intra_splat_depth = texture2D(depthImg, tc).r + (texture2D(depthImg, tc).g + texture2D(depthImg, tc).b/255.0)/255.0;
    if ( intra_splat_depth > 0.999 ) {
	if (splatSetIndex==-1) {
	    // We treat this "most recent model" specially, so that it can overwrite spurious fragments from the other models.
	    // For the other models, we want a 'discard', to give better models a chance of colouring this fragment, but
	    // when the "most recent model" is rendered, it should override anything else.
	    gl_FragColor = vec4( backgroundCol, src_alpha );
	    return;
	}
	discard;
    }

    // If this is "the most recent proxy model", it should be brought some amount toward the front. Note that this is
    // not a very good solution for billboards, since it requires a very large offset to work properly. Possible
    // solutions to this: 1) Use non-gl_Point-primitives that have "transformed fragments", or 2) set depths using the
    // gl_FragDepth extension. (Don't think (1) is a good idea, will not work for non-planar regions.) Using a minimal
    // offset if we don't have the gl_FragDepth extension.
#ifdef USE_FRAG_DEPTH_EXT
    // 140622: No need to offset.
    //         Was it a mistake not to use an offset here? 
#else
    if (splatSetIndex==-1) {
	intra_splat_depth = clamp(intra_splat_depth - 0.001*float(mostRecentProxyModelOffset), 0.0, 1.0);
    }
#endif

    // If the assumed-locally-planar geometry and the measured intra-splat depth deviates too much, the intra-splat
    // texture lookups will simply be wrong, so we might as well discard these fragments. Note that this is not a sure
    // proof way of detecting all splat fragments that are outside the "good" region, only that if this test comes out
    // true, we certainly should discard the fragment. (Where this is insufficient, is probably in cases where the
    // intra_splat_depth gets sampled from a "wrong" location still in the planar region, thus causing a failure to
    // produce a depth deviation signalling a fragment to be discarded. Maybe this is more prone to happen for
    // geometries with large planar parts?!)
    highp float planar_depth = sampled_depth + dot( depth_e, intraSplatTexCooTransform2*vec2(c.x, -c.y) );
    // To visualize the difference between the assumed-locally-planar geometry and the measured intra-splat depth:
    // gl_FragColor = vec4( 1000.0*abs(planar_depth-intra_splat_depth)*vec3(1.0), src_alpha ); return;

#ifdef DEBUG
    if (useISTC>0) {
        if ( abs(planar_depth-intra_splat_depth) > 0.5/1000.0 ) // Values chosen by using the visualization above
            discard;
    }
    if (decayMode==0) {
        decay = 1.0;
    }
    gl_FragColor = vec4( decay * texture2D( rgbImage, tc ).xyz, src_alpha );
    if (debugSplatCol>0) {
        if (splatSetIndex==0)  gl_FragColor = vec4(decay, 0.0, 0.0, src_alpha);
        if (splatSetIndex==1)  gl_FragColor = vec4(0.0, decay, 0.0, src_alpha);
        if (splatSetIndex==2)  gl_FragColor = vec4(0.0, 0.0, decay, src_alpha);
        if (splatSetIndex==3)  gl_FragColor = vec4(decay, decay, 0.0, src_alpha); // 3) yellow?
        if (splatSetIndex==4)  gl_FragColor = vec4(0.0, decay, decay, src_alpha); // 4) cyan?
        if (splatSetIndex==5)  gl_FragColor = vec4(decay, 0.0, decay, src_alpha); // 5) magenta?
        if (splatSetIndex==6)  gl_FragColor = vec4(0.5*decay, 0.0, 0.0, src_alpha);
        if (splatSetIndex==7)  gl_FragColor = vec4(0.0, 0.5*decay, 0.0, src_alpha);
        if (splatSetIndex==8)  gl_FragColor = vec4(0.0, 0.0, 0.5*decay, src_alpha);
        if (splatSetIndex==9)  gl_FragColor = vec4(0.5*decay, 0.5*decay, 0.0, src_alpha);
        if (splatSetIndex==-1) {
	    //if ( r_squared > 0.16 ) gl_FragColor = vec4(1.0, 1.0, 1.0, src_alpha); else gl_FragColor = vec4(1.0, 0.0, 0.0, src_alpha);
            highp float x = floor(0.1*(gl_FragCoord.x + 1.0) * float(vp_width) + 0.5);
            x = x-2.0*floor(x/2.0);
            highp float r = 0.0;
            if ( x > 0.5 ) r = 1.0;
	    if ( r_squared > 0.16 ) gl_FragColor = vec4(1.0, 1.0, 1.0, src_alpha); else gl_FragColor = vec4(r, 0.0, 0.0, src_alpha);
        }
    } else {
        // To visualize the "most recent proxy model" even when not in debug-colour-mode:
	//         if (splatSetIndex==-1)
	//             if ( ( r_squared > 0.2 ) && ( r_squared < 0.25 ) )
	//                 gl_FragColor = vec4(1.0, 1.0, 1.0, src_alpha);
    }
    if (splatOutline>0) {
	highp float tmp = 0.5-0.0015*float(splats_x)/actualSplatOverlap;
	if ( (c.x<-tmp) || (c.x>tmp) || (c.y<-tmp) || (c.y>tmp) )
	    gl_FragColor = vec4(1.0, 1.0, 1.0, src_alpha);
    }
#else
    if ( abs(planar_depth-intra_splat_depth) > 0.5/1000.0 ) { // Values chosen by using the visualization above
	discard;
    }
    gl_FragColor = vec4( texture2D( rgbImage, tc ).xyz, src_alpha );
#endif
    
#ifdef USE_FRAG_DEPTH_EXT
    // With the help of 'frag_depth', 'frag_depth_e', 'intraSplatTexCooTransform2' and the intra-splat coordinates 'c',
    // we can interpolate the appropriate depth buffer value for the planar part of the splat. If the gl_FragDepthEXT
    // extension is available, we can set the depth value to this, and this makes it much more effective to use an
    // offset to force the most recent proxy model in front.
    
    highp float planar_frag_depth = frag_depth + dot( frag_depth_e, intraSplatTexCooTransform2*vec2(c.x, -c.y) );
    
    // Testing. Should give black in center. Then the interpolated z-frag is correct there. Should then be red
    // farther away, and green closer.
    // gl_FragColor = vec4(500.0*clamp(planar_frag_depth-depth, 0.0, 1.0), 500.0*clamp(depth-planar_frag_depth, 0.0, 1.0), 0.0, src_alpha); return;
    
    if (splatSetIndex==-1) {
        planar_frag_depth = clamp(planar_frag_depth - 0.001*float(mostRecentProxyModelOffset), 0.0, 1.0);
    }

    // This will cause proxy models to be layered, and not flicker in and out due to small differences in computed frag.z values
    // if (splatSetIndex>=0) planar_frag_depth = clamp(planar_frag_depth + 0.001*float(splatSetIndex), 0.0, 1.0);

#ifdef DEBUG        
    if (useISTC>0) {
	gl_FragDepthEXT = planar_frag_depth;
    } else {
	gl_FragDepthEXT = frag_depth; // NB! All paths in the FS must set this, if any at all!
    }
#else
    gl_FragDepthEXT = planar_frag_depth;
#endif

#endif

    // To identify particular splats during debugging (and figure generation)
    // if (    ( abs(splat_i-43.0) < 0.5 )   &&    ( abs(splat_j-43.0) < 0.5 )    )
    //     gl_FragColor = vec4(1.0, 1.0, 1.0, src_alpha);
    // if (    ( abs(splat_i-42.0) < 0.5 )   &&    ( abs(splat_j-43.0) < 0.5 )    )
    //     gl_FragColor = vec4(0.0, 1.0, 0.0, src_alpha);
    // if (    ( abs(splat_i-43.0) < 0.5 )   &&    ( abs(splat_j-44.0) < 0.5 )    )
    //     gl_FragColor = vec4(0.0, 0.0, 1.0, src_alpha);

}
