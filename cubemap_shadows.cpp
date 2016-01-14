#include "framework.h"
#include "intersection.h"

using namespace prototyper;

int main( int argc, char** argv )
{
  shape::set_up_intersection();

  map<string, string> args;

  for( int c = 1; c < argc; ++c )
  {
    args[argv[c]] = c + 1 < argc ? argv[c + 1] : "";
    ++c;
  }

  cout << "Arguments: " << endl;
  for_each( args.begin(), args.end(), []( pair<string, string> p )
  {
    cout << p.first << " " << p.second << endl;
  } );

  uvec2 screen( 0 );
  bool fullscreen = false;
  bool silent = false;
  string title = "Basic CPP to get started";

  /*
   * Process program arguments
   */

  stringstream ss;
  ss.str( args["--screenx"] );
  ss >> screen.x;
  ss.clear();
  ss.str( args["--screeny"] );
  ss >> screen.y;
  ss.clear();

  if( screen.x == 0 )
  {
    screen.x = 1280;
  }

  if( screen.y == 0 )
  {
    screen.y = 720;
  }

  try
  {
    args.at( "--fullscreen" );
    fullscreen = true;
  }
  catch( ... ) {}

  try
  {
    args.at( "--help" );
    cout << title << ", written by Marton Tamas." << endl <<
         "Usage: --silent      //don't display FPS info in the terminal" << endl <<
         "       --screenx num //set screen width (default:1280)" << endl <<
         "       --screeny num //set screen height (default:720)" << endl <<
         "       --fullscreen  //set fullscreen, windowed by default" << endl <<
         "       --help        //display this information" << endl;
    return 0;
  }
  catch( ... ) {}

  try
  {
    args.at( "--silent" );
    silent = true;
  }
  catch( ... ) {}

  /*
   * Initialize the OpenGL context
   */

  framework frm;
  frm.init( screen, title, fullscreen );
  frm.set_vsync( true );

  //set opengl settings
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LEQUAL );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  glClearColor( 0.1f, 0.2f, 0.3f, 1.0f );
  glClearDepth( 1.0f );

  frm.get_opengl_error();

  /*
   * Set up mymath
   */

  camera<float> cam;
  frame<float> the_frame;

  frame<float> shadow_frame;
  shadow_frame.set_perspective( radians( 90 ), 1, 1, 100 );

  float cam_fov = 45.0f;
  float cam_near = 1.0f;
  float cam_far = 100.0f;

  the_frame.set_perspective( radians( cam_fov ), ( float )screen.x / ( float )screen.y, cam_near, cam_far );

  glViewport( 0, 0, screen.x, screen.y );

  /*
   * Set up the scene
   */

  float move_amount = 5;
  float cam_rotation_amount = 5.0;

  scene s;
  mesh::load_into_meshes( "../resources/cubemap_shadow/mesh.obj", s );
  int counter = 0;
  for( auto& i : s.meshes )
    i.write_mesh( "../resources/cubemap_shadow/mesh" + std::to_string(counter++) + ".mesh" );

  int nummeshes = 13;

  vector<mesh> scene;
  scene.resize( nummeshes );

  for( int c = 0; c < nummeshes; ++c )
  {
    stringstream ss;
    ss << "../resources/cubemap_shadow/mesh" << c << ".mesh";
    scene[c].read_mesh( ss.str() );
  }

for( auto & c : scene )
    c.upload();

  GLuint the_box = frm.create_box();

  /*
   * Set up the shaders
   */

  GLuint lighting_shader = 0;
  frm.load_shader( lighting_shader, GL_VERTEX_SHADER, "../shaders/cubemap_shadow/lighting.vs" );
  frm.load_shader( lighting_shader, GL_FRAGMENT_SHADER, "../shaders/cubemap_shadow/lighting.ps" );

  GLuint shadow_gen_shader = 0;
  frm.load_shader( shadow_gen_shader, GL_VERTEX_SHADER, "../shaders/cubemap_shadow/shadow_gen.vs" );
  frm.load_shader( shadow_gen_shader, GL_GEOMETRY_SHADER, "../shaders/cubemap_shadow/shadow_gen.gs" );
  frm.load_shader( shadow_gen_shader, GL_FRAGMENT_SHADER, "../shaders/cubemap_shadow/shadow_gen.ps" );

  GLuint debug_shader = 0;
  frm.load_shader( debug_shader, GL_VERTEX_SHADER, "../shaders/cubemap_shadow/cubemap_debug.vs" );
  frm.load_shader( debug_shader, GL_FRAGMENT_SHADER, "../shaders/cubemap_shadow/cubemap_debug.ps" );

  GLint lighting_mv_mat_loc = glGetUniformLocation( lighting_shader, "mv" );
  GLint lighting_p_mat_loc = glGetUniformLocation( lighting_shader, "p" );
  GLint lighting_normal_mat_loc = glGetUniformLocation( lighting_shader, "normal_mat" );
  GLint lighting_pos_loc = glGetUniformLocation( lighting_shader, "light_pos" );
  GLint lighting_radius_loc = glGetUniformLocation( lighting_shader, "radius" );
  GLint lighting_inv_view_loc = glGetUniformLocation( lighting_shader, "inv_view" );
  GLint lighting_viewproj_mat_loc = glGetUniformLocation( lighting_shader, "cube_viewproj" );
  GLint lighting_model_light_pos_loc = glGetUniformLocation( lighting_shader, "model_light_pos" );

  GLint shadow_viewproj_mat_loc = glGetUniformLocation( shadow_gen_shader, "cube_viewproj" );
  GLint shadow_model_mat_loc = glGetUniformLocation( shadow_gen_shader, "light_model_mat" );

  GLint debug_mvp_mat_loc = glGetUniformLocation( debug_shader, "mvp" );

  /*
   * Set up textures
   */

  GLuint cubemap_tex;
  glGenTextures( 1, &cubemap_tex );
  glBindTexture( GL_TEXTURE_CUBE_MAP, cubemap_tex );

  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

  uvec2 texsize( 2048 );

  for( int c = 0; c < 6; ++c )
    glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + c, 0, GL_DEPTH_COMPONENT24, texsize.x, texsize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0 );

  GLuint cubemap_fbo;
  glGenFramebuffers( 1, &cubemap_fbo );
  glBindFramebuffer( GL_FRAMEBUFFER, cubemap_fbo );

  //attach depth cubemap to fbo's depth attachment
  glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubemap_tex, 0 );

  //no color textures attached
  glDrawBuffer( GL_NONE );

  frm.check_fbo_status();

  //unbind
  glBindFramebuffer( GL_FRAMEBUFFER, 0 );

  /*
   * Set up shadow view matrices
   */

  vector<mat4> shadow_mats;

  //pos x
  shadow_mats.push_back( mat4( 0, 0, -1, 0,
                               0, -1, 0, 0,
                               -1, 0, 0, 0,
                               0, 0, 0, 1 ) );
  //neg x
  shadow_mats.push_back( mat4( 0, 0, 1, 0,
                               0, -1, 0, 0,
                               1, 0, 0, 0,
                               0, 0, 0, 1 ) );
  //pos y
  shadow_mats.push_back( mat4( 1, 0, 0, 0,
                               0, 0, -1, 0,
                               0, 1, 0, 0,
                               0, 0, 0, 1 ) );
  //neg y
  shadow_mats.push_back( mat4( 1, 0, 0, 0,
                               0, 0, 1, 0,
                               0, -1, 0, 0,
                               0, 0, 0, 1 ) );
  //pos z
  shadow_mats.push_back( mat4( 1, 0, 0, 0,
                               0, -1, 0, 0,
                               0, 0, -1, 0,
                               0, 0, 0, 1 ) );
  //neg z
  shadow_mats.push_back( mat4( -1, 0, 0, 0,
                               0, -1, 0, 0,
                               0, 0, 1, 0,
                               0, 0, 0, 1 ) );

  for( auto & c : shadow_mats )
    c = shadow_frame.projection_matrix * c;

  /*
   * Handle events
   */

  bool warped = false, ignore = true;
  vec2 movement_speed = vec2(0);

  auto event_handler = [&]( const sf::Event & ev )
  {
    switch( ev.type )
    {
      case sf::Event::MouseMoved:
        {
          vec2 mpos( ev.mouseMove.x / float( screen.x ), ev.mouseMove.y / float( screen.y ) );

          if( warped )
          {
            ignore = false;
          }
          else
          {
            frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
            warped = true;
            ignore = true;
          }

          if( !ignore && all( notEqual( mpos, vec2( 0.5 ) ) ) )
          {
            cam.rotate( mm::radians( -180.0f * ( mpos.x - 0.5f ) ), mm::vec3( 0.0f, 1.0f, 0.0f ) );
            cam.rotate_x( mm::radians( -180.0f * ( mpos.y - 0.5f ) ) );
            frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
            warped = true;
          }
        }
      case sf::Event::KeyPressed:
        {
          /*if( ev.key.code == sf::Keyboard::A )
          {
            cam.rotate_y( radians( cam_rotation_amount ) );
          }*/
        }
      default:
        break;
    }
  };

  /*
   * Render
   */

  sf::Clock timer;
  timer.restart();

  sf::Clock movement_timer;
  movement_timer.restart();

  vec3 light_pos = vec3( 0, 0, 0 );
  vec3 light_velocity = normalize( vec3( frm.get_random_num( 0, 1 ), frm.get_random_num( 0, 1 ), frm.get_random_num( 0, 1 ) ) ) * 5;
  vec3 light_movement = vec3( 0 );
  float light_movement_amount = 5;

  vector<camera<float> > light_cams;
  light_cams.resize( 6 );

  frustum camera_frustum;

  vector<frustum> light_frustums;
  light_frustums.resize( 6 );

  vector<int> cull_res;
  cull_res.resize( 6 );

  frm.display( [&]
  {
    frm.handle_events( event_handler );

    float seconds = movement_timer.getElapsedTime().asMilliseconds() / 1000.0f;

    if( seconds > 0.01667 )
    {
      //move light
      light_pos += light_velocity * 1 / 60.0f;

      float halfFieldWidth = 5;
      float halfFieldHeight = 5;

      vec3 normal = vec3( 0, 0, 0 );
      if( light_pos.x <= -halfFieldWidth )
      {
        normal = vec3( 1, 0, 0 );
      }
      else if( light_pos.x >= halfFieldWidth )
      {
        normal = vec3( -1, 0, 0 );
      }
      else if( light_pos.y <= -halfFieldHeight )
      {
        normal = vec3( 0, 1, 0 );
      }
      else if( light_pos.y >= halfFieldHeight )
      {
        normal = vec3( 0, -1, 0 );
      }
      else if( light_pos.z <= -halfFieldHeight )
      {
        normal = vec3( 0, 0, 1 );
      }
      else if( light_pos.z >= halfFieldHeight )
      {
        normal = vec3( 0, 0, -1 );
      }

      if( normal.x != 0 || normal.y != 0 || normal.z != 0 )
      {
        light_velocity = reflect( light_velocity, normal );
      }


      //move camera
      if( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) )
      {
        movement_speed.x -= move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) )
      {
        movement_speed.x += move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) )
      {
        movement_speed.y += move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) )
      {
        movement_speed.y -= move_amount;
      }

      cam.move_forward( movement_speed.y * seconds );
      cam.move_right( movement_speed.x * seconds );
      movement_speed *= 0.5;

      //move light
      if( sf::Keyboard::isKeyPressed( sf::Keyboard::J ) )
      {
        light_movement.x -= light_movement_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::L ) )
      {
        light_movement.x += light_movement_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::I ) )
      {
        light_movement.z -= light_movement_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::K ) )
      {
        light_movement.z += light_movement_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::U ) )
      {
        light_movement.y -= light_movement_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::O ) )
      {
        light_movement.y += light_movement_amount;
      }

      light_pos.z += ( light_movement.z * seconds );
      light_pos.x += ( light_movement.x * seconds );
      light_pos.y += ( light_movement.y * seconds );
      light_movement *= 0.5;

      movement_timer.restart();
    }

    for( auto& c : light_cams )
    {
      c = camera<float>();
      c.move_forward( light_pos.z );
      c.move_up( light_pos.y );
      c.move_right( light_pos.x );
    }

    light_cams[0].rotate_y( radians( 90 ) ); //posx
    light_cams[1].rotate_y( radians( -90 ) ); //negx

    light_cams[2].rotate_x( radians( 90 ) ); //posy
    light_cams[3].rotate_x( radians( 90 ) ); //negy

    light_cams[4].rotate_y( radians( 180 ) ); //posz
    //light_cams[5].rotate_y(radians(90)); //negz

    camera_frustum.set_up( cam, the_frame );

    int cnt = 0;

    for( auto& c : light_frustums )
      c.set_up( light_cams[cnt++], shadow_frame );

    int passed = 0;

    /*cnt = 0;
    for( auto& c : light_frustums )
    {
      cull_res[cnt] = 1;
      cull_res[cnt] &= cull_res[cnt] ? camera_frustum.intersects( c.planes[0] ) : 0;
      cull_res[cnt] &= cull_res[cnt] ? camera_frustum.intersects( c.planes[1] ) : 0;
      cull_res[cnt] &= cull_res[cnt] ? camera_frustum.intersects( c.planes[2] ) : 0;
      cull_res[cnt] &= cull_res[cnt] ? camera_frustum.intersects( c.planes[3] ) : 0;
      cull_res[cnt] &= cull_res[cnt] ? camera_frustum.intersects( c.planes[4] ) : 0;
      cull_res[cnt] &= cull_res[cnt] ? camera_frustum.intersects( c.planes[5] ) : 0;

      if( cull_res[cnt] )
        passed++;

      ++cnt;
    }

    int a = 0;*/

    char title[20];
    itoa( passed, title, 10 );
    frm.set_title( title );

    /**
     * Render
     */

    mat4 light_model_mat = create_translation( -light_pos.xyz );
    float radius = 100;

    //render to shadow map
    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    glViewport( 0, 0, texsize.x, texsize.y );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

    //render shadow map
    glBindFramebuffer( GL_FRAMEBUFFER, cubemap_fbo );
    glClear( GL_DEPTH_BUFFER_BIT );

    glUseProgram( shadow_gen_shader );
    glUniformMatrix4fv( shadow_viewproj_mat_loc, 6, false, &shadow_mats[0][0][0] );
    glUniformMatrix4fv( shadow_model_mat_loc, 1, false, &light_model_mat[0][0] );

  for( auto& c : scene )
      c.render();

    //render scene
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glViewport( 0, 0, screen.x, screen.y );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( lighting_shader );

    mat4 view = cam.get_matrix();
    mat4 model = mat4::identity;
    mat4 projection = the_frame.projection_matrix;
    mat4 mv = view * model;
    mat4 normal_mat = mv;
    mat4 mvp = projection * mv;
    glUniformMatrix4fv( lighting_mv_mat_loc, 1, false, &mv[0][0] );
    glUniformMatrix4fv( lighting_p_mat_loc, 1, false, &projection[0][0] );
    glUniformMatrix4fv( lighting_normal_mat_loc, 1, false, &normal_mat[0][0] );
    glUniform4fv( lighting_model_light_pos_loc, 1, &light_pos.x );
    vec4 vs_light_pos = mv* vec4(light_pos,1);
    glUniform4fv( lighting_pos_loc, 1, &vs_light_pos.x );
    glUniform1fv( lighting_radius_loc, 1, &radius );
    mat4 inv_view = inverse( view );
    glUniformMatrix4fv( lighting_inv_view_loc, 1, false, &inv_view[0][0] );
    glUniformMatrix4fv( lighting_viewproj_mat_loc, 6, false, &shadow_mats[0][0][0] );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, cubemap_tex );

  for( auto& c : scene )
      c.render();

    //debug the cubemap
    glUseProgram( debug_shader );

    mat4 trans = create_translation( light_pos.xyz );
    mv = mv * trans;
    mvp = projection * mv;
    glUniformMatrix4fv( debug_mvp_mat_loc, 1, false, &mvp[0][0] );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, cubemap_tex );

    glBindVertexArray( the_box );
    glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

    frm.get_opengl_error();
  }, silent );

  return 0;
}
