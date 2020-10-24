#include <iostream>
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

void printWorkGroupCount()
{
    int work_grp_cnt[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    
    std::cout << "max global (total) work group size x: "  << work_grp_cnt[0]
                                                 << " y: " << work_grp_cnt[1]
                                                 << " z: " << work_grp_cnt[2] << std::endl << std::endl; 
}

void printWorkGroupSize()
{
    int work_grp_size[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

    std::cout << "max local (in order) work group sizes x: "  << work_grp_size[0]
                                                    << " y: " << work_grp_size[1]
                                                    << " z: " << work_grp_size[2] << std::endl << std::endl; 
}

int printInvocations()
{
    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);

    std::cout << "max local work group invocations " << work_grp_inv << std::endl << std::endl;

    return work_grp_inv;
}