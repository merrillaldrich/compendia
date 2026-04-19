/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Compendia", "index.html", [
    [ "CLAUDE.md", "md__c_l_a_u_d_e.html", [
      [ "About", "md__c_l_a_u_d_e.html#autotoc_md1", null ],
      [ "Build", "md__c_l_a_u_d_e.html#autotoc_md2", null ],
      [ "Architecture", "md__c_l_a_u_d_e.html#autotoc_md3", null ],
      [ "Documentation", "md__c_l_a_u_d_e.html#autotoc_md4", null ],
      [ "Key dependencies", "md__c_l_a_u_d_e.html#autotoc_md5", null ]
    ] ],
    [ "Packaging", "md__p_a_c_k_a_g_i_n_g.html", [
      [ "Windows — Self-Contained ZIP", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md8", [
        [ "Prerequisites", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md9", null ],
        [ "Steps", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md10", [
          [ "1. Configure with CMake (Release)", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md11", null ],
          [ "2. Build", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md12", null ],
          [ "3. Create the ZIP", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md13", null ]
        ] ],
        [ "What the package contains", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md14", null ],
        [ "Deployment", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md15", null ],
        [ "Troubleshooting", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md16", null ]
      ] ],
      [ "Windows — MSIX (Microsoft Store)", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md18", [
        [ "Prerequisites", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md19", null ],
        [ "Steps", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md20", [
          [ "1. Build in Qt Creator", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md21", null ],
          [ "2. Run the packaging script", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md22", null ],
          [ "3. Submit to Partner Center", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md23", null ]
        ] ],
        [ "Updating the version", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md24", null ],
        [ "<span class=\"tt\">broadFileSystemAccess</span> justification", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md25", null ],
        [ "Testing locally", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md26", null ],
        [ "Troubleshooting", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md27", null ]
      ] ],
      [ "Linux — AppImage (Self-Contained)", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md29", [
        [ "Prerequisites", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md30", null ],
        [ "Steps", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md31", [
          [ "1. Build in Qt Creator", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md32", null ],
          [ "2. Build the AppImage", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md33", null ]
        ] ],
        [ "What the package contains", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md34", null ],
        [ "Deployment", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md35", null ],
        [ "Uploading to a GitHub Release", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md36", null ],
        [ "Troubleshooting", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md37", null ]
      ] ],
      [ "Linux — TGZ Archive (requires system Qt)", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md39", [
        [ "Prerequisites", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md40", null ],
        [ "Prerequisites", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md41", null ],
        [ "Steps", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md42", [
          [ "1. Build in Qt Creator", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md43", null ],
          [ "2. Configure with CMake (if not already done)", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md44", null ],
          [ "3. Create the TGZ", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md45", null ]
        ] ],
        [ "What the package contains", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md46", null ],
        [ "Deployment", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md47", null ],
        [ "Uploading to a GitHub Release", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md48", null ],
        [ "Troubleshooting", "md__p_a_c_k_a_g_i_n_g.html#autotoc_md49", null ]
      ] ]
    ] ],
    [ "compendia", "md__r_e_a_d_m_e.html", [
      [ "Features", "md__r_e_a_d_m_e.html#autotoc_md52", [
        [ "Cross-Platform", "md__r_e_a_d_m_e.html#autotoc_md53", null ],
        [ "Browsing &amp; Preview", "md__r_e_a_d_m_e.html#autotoc_md54", null ],
        [ "Tagging", "md__r_e_a_d_m_e.html#autotoc_md55", null ],
        [ "Filtering", "md__r_e_a_d_m_e.html#autotoc_md56", null ],
        [ "EXIF Metadata", "md__r_e_a_d_m_e.html#autotoc_md57", null ],
        [ "Star Ratings", "md__r_e_a_d_m_e.html#autotoc_md58", null ],
        [ "Navigation &amp; Isolation", "md__r_e_a_d_m_e.html#autotoc_md59", null ],
        [ "Autos", "md__r_e_a_d_m_e.html#autotoc_md60", null ],
        [ "Face Detection", "md__r_e_a_d_m_e.html#autotoc_md61", null ],
        [ "Export", "md__r_e_a_d_m_e.html#autotoc_md62", null ]
      ] ],
      [ "Architecture", "md__r_e_a_d_m_e.html#autotoc_md64", null ],
      [ "Building", "md__r_e_a_d_m_e.html#autotoc_md66", [
        [ "Prerequisites", "md__r_e_a_d_m_e.html#autotoc_md67", null ],
        [ "Configure and build", "md__r_e_a_d_m_e.html#autotoc_md68", null ]
      ] ],
      [ "Tag persistence format", "md__r_e_a_d_m_e.html#autotoc_md70", null ],
      [ "License", "md__r_e_a_d_m_e.html#autotoc_md72", null ]
    ] ],
    [ "Versioning", "md__v_e_r_s_i_o_n_i_n_g.html", [
      [ "Overview", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md74", null ],
      [ "Release workflow", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md75", [
        [ "1. Finish the work and commit everything", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md76", null ],
        [ "2. Tag the release commit", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md77", null ],
        [ "3. Build and package from the tagged commit", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md78", null ],
        [ "4. Advance the version in CMakeLists.txt", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md79", null ]
      ] ],
      [ "Publishing a GitHub Release", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md80", null ],
      [ "Choosing version numbers", "md__v_e_r_s_i_o_n_i_n_g.html#autotoc_md81", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"aboutdialog_8cpp.html",
"class_nav_filter_container.html#a4d697feda26ed061165d2aaa7c3fcfc6",
"class_zoomable_graphics_view.html#ab989c337d1ab61581f5938adedc4f899",
"struct_face_worker_pool.html#a44cee03bcd0f3b6e5564b5f426da0cff"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';